/**
 * Copyright © 2017-2026 Wellington Wallace
 *
 * This file is part of Easy Effects.
 *
 * Easy Effects is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Easy Effects is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Easy Effects. If not, see <https://www.gnu.org/licenses/>.
 */

#include "tape_echo.hpp"
#include <qapplication.h>
#include <qnamespace.h>
#include <qobjectdefs.h>
#include <QTimer>
#include <cstdint>
#include <algorithm>
#include <format>
#include <memory>
#include <mutex>
#include <span>
#include <string>
#include "db_manager.hpp"
#include "easyeffects_db_tape_echo.h"
#include "lv2_macros.hpp"
#include "lv2_wrapper.hpp"
#include "pipeline_type.hpp"
#include "plugin_base.hpp"
#include "pw_manager.hpp"
#include "tags_plugin_name.hpp"
#include "util.hpp"

TapeEcho::TapeEcho(const std::string& tag, pw::Manager* pipe_manager, PipelineType pipe_type, QString instance_id)
    : PluginBase(tag,
                 tags::plugin_name::BaseName::tape_echo,
                 tags::plugin_package::Package::calf,
                 instance_id,
                 pipe_manager,
                 pipe_type),
      settings(db::Manager::self().get_plugin_db<DbTapeEcho>(pipe_type,
                                                            tags::plugin_name::BaseName::tape_echo + "#" + instance_id)) {
  const auto lv2_plugin_uri = "http://calf.sourceforge.net/plugins/VintageDelay";

  echo_wrapper = std::make_unique<lv2::Lv2Wrapper>(lv2_plugin_uri);

  packageInstalled = echo_wrapper->found_plugin;

  if (!packageInstalled) {
    util::debug(std::format("{}{} is not installed", log_tag, lv2_plugin_uri));
  }

  static_assert(std::atomic<float>::is_always_lock_free);
  static_assert(std::atomic<uint64_t>::is_always_lock_free);
  static_assert(std::atomic<uint32_t>::is_always_lock_free);

  bypass = settings->bypass();
  echo_input_gain = util::db_to_linear(settings->inputGain());
  echo_output_gain = util::db_to_linear(settings->outputGain());
  connect(settings, &DbTapeEcho::bypassChanged, this, [this] { bypass = settings->bypass(); });
  connect(settings, &DbTapeEcho::inputGainChanged, this,
          [this] { echo_input_gain = util::db_to_linear(settings->inputGain()); });
  connect(settings, &DbTapeEcho::outputGainChanged, this,
          [this] { echo_output_gain = util::db_to_linear(settings->outputGain()); });

  for (const auto* port : {"on", "timing", "subdiv", "time_l", "time_r"}) {
    echo_wrapper->set_control_port_value(port, 1.0F);
  }
  echo_wrapper->set_control_port_value("mix_mode", 0.0F);
  const std::array<const char*, 5> symbols{"ms", "feedback", "medium", "dry", "amount"};
  for (size_t i = 0; i < symbols.size(); ++i) {
    for (auto& port : echo_wrapper->ports) {
      if (port.type == lv2::PortType::TYPE_CONTROL && port.is_input && port.symbol == symbols[i]) {
        control_ports[i] = &port;
        break;
      }
    }
  }
#define BIND_PENDING(index, getter, signal) \
  pending_controls[index].store(static_cast<float>(settings->getter())); \
  connect(settings, &DbTapeEcho::signal, this, [this] { \
    pending_controls[index].store(static_cast<float>(settings->getter()), std::memory_order_relaxed); \
  })
  BIND_PENDING(0, delayTime, delayTimeChanged);
  BIND_PENDING(1, feedback, feedbackChanged);
  BIND_PENDING(2, medium, mediumChanged);
  BIND_PENDING(3, dry, dryChanged);
  BIND_PENDING(4, wet, wetChanged);
#undef BIND_PENDING

  // This timer lives on the existing worker, never on the PipeWire thread.
  QMetaObject::invokeMethod(baseWorker, [this] {
    auto* timer = new QTimer(baseWorker);
    connect(timer, &QTimer::timeout, baseWorker, [this] { service_instance(); });
    timer->start(20);
  }, Qt::QueuedConnection);
}

TapeEcho::~TapeEcho() {
  if (connected_to_pw) {
    disconnect_from_pw();
  }

  stop_worker();

  settings->disconnect();

  util::debug(std::format("{}{} destroyed", log_tag, name.toStdString()));
}

void TapeEcho::reset() {
  settings->setDefaults();
}

void TapeEcho::clear_data() {
  reset_generation.fetch_add(1, std::memory_order_relaxed);
}

void TapeEcho::setup() {
  // Called from PipeWire's realtime callback. Publish only; worker does lifecycle work.
  const auto config = (static_cast<uint64_t>(rate) << 32U) | n_samples;
  requested_config.store(config, std::memory_order_release);
}

void TapeEcho::service_instance() {
  const auto config = requested_config.load(std::memory_order_acquire);
  const auto reset = reset_generation.load(std::memory_order_relaxed);
  if (config == 0 || (config == applied_config && reset == applied_reset)) {
    return;
  }
  if (instance_gate.test_and_set(std::memory_order_acquire)) {
    return;
  }
  // The gate excludes connect_port/run, and destruction occurs only here.
  echo_wrapper->destroy_instance_locked();
  echo_wrapper->set_n_samples(static_cast<uint>(config));
  instance_ready = echo_wrapper->found_plugin && echo_wrapper->create_instance(static_cast<uint>(config >> 32U));
  applied_config = config;
  applied_reset = reset;
  instance_gate.clear(std::memory_order_release);
}

void TapeEcho::process(std::span<float>& left_in,
                      std::span<float>& right_in,
                      std::span<float>& left_out,
                      std::span<float>& right_out) {
  if (bypass) {
    std::ranges::copy(left_in, left_out.begin());
    std::ranges::copy(right_in, right_out.begin());

    return;
  }

  const auto output_gain = echo_output_gain.load(std::memory_order_relaxed);
  const auto config = requested_config.load(std::memory_order_acquire);
  if (instance_gate.test_and_set(std::memory_order_acquire)) {
    std::ranges::copy(left_in, left_out.begin());
    std::ranges::copy(right_in, right_out.begin());
    if (output_gain != 1.0F) {
      apply_gain(left_out, right_out, output_gain);
    }
    return;
  }

  if (!instance_ready || applied_config != config ||
      applied_reset != reset_generation.load(std::memory_order_relaxed)) {
    instance_gate.clear(std::memory_order_release);
    std::ranges::copy(left_in, left_out.begin());
    std::ranges::copy(right_in, right_out.begin());
    if (output_gain != 1.0F) {
      apply_gain(left_out, right_out, output_gain);
    }
    return;
  }

  for (size_t i = 0; i < control_ports.size(); ++i) {
    if (auto* port = control_ports[i]; port != nullptr) {
      port->value = std::clamp(pending_controls[i].load(std::memory_order_relaxed), port->min, port->max);
    }
  }
  const auto input_gain = echo_input_gain.load(std::memory_order_relaxed);
  if (input_gain != 1.0F) {
    apply_gain(left_in, right_in, input_gain);
  }
  echo_wrapper->connect_data_ports(left_in, right_in, left_out, right_out);
  echo_wrapper->run();
  instance_gate.clear(std::memory_order_release);

  if (output_gain != 1.0F) {
    apply_gain(left_out, right_out, output_gain);
  }

  if (updateLevelMeters) {
    get_peaks(left_in, right_in, left_out, right_out);
  }
}

void TapeEcho::process([[maybe_unused]] std::span<float>& left_in,
                      [[maybe_unused]] std::span<float>& right_in,
                      [[maybe_unused]] std::span<float>& left_out,
                      [[maybe_unused]] std::span<float>& right_out,
                      [[maybe_unused]] std::span<float>& probe_left,
                      [[maybe_unused]] std::span<float>& probe_right) {}

auto TapeEcho::get_latency_seconds() -> float {
  return 0.0F;
}
