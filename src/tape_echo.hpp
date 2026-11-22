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

#pragma once

#include <qqmlintegration.h>
#include <qtmetamacros.h>
#include <QString>
#include <span>
#include <array>
#include <atomic>
#include <cstdint>
#include <memory>
#include <string>
#include "easyeffects_db_tape_echo.h"
#include "pipeline_type.hpp"
#include "plugin_base.hpp"
#include "pw_manager.hpp"

class TapeEcho : public PluginBase {
  Q_OBJECT
  QML_NAMED_ELEMENT(BackendTapeEcho)
  QML_UNCREATABLE("Use the c++ instance")

 public:
  TapeEcho(const std::string& tag, pw::Manager* pipe_manager, PipelineType pipe_type, QString instance_id);
  TapeEcho(const TapeEcho&) = delete;
  auto operator=(const TapeEcho&) -> TapeEcho& = delete;
  TapeEcho(const TapeEcho&&) = delete;
  auto operator=(const TapeEcho&&) -> TapeEcho& = delete;
  ~TapeEcho() override;

  void reset() override;

  void clear_data() override;

  void setup() override;

  void process(std::span<float>& left_in,
               std::span<float>& right_in,
               std::span<float>& left_out,
               std::span<float>& right_out) override;

  void process(std::span<float>& left_in,
               std::span<float>& right_in,
               std::span<float>& left_out,
               std::span<float>& right_out,
               std::span<float>& probe_left,
               std::span<float>& probe_right) override;

  auto get_latency_seconds() -> float override;

 private:
  DbTapeEcho* settings = nullptr;

  // The audio callback only tries the gate once; the worker never waits on it.
  std::atomic_flag instance_gate = ATOMIC_FLAG_INIT;
  std::atomic<uint64_t> requested_config{0};
  std::atomic<uint32_t> reset_generation{0};
  uint64_t applied_config = 0;
  uint32_t applied_reset = 0;
  bool instance_ready = false;
  std::unique_ptr<lv2::Lv2Wrapper> echo_wrapper;
  std::array<std::atomic<float>, 5> pending_controls{};
  std::array<lv2::Port*, 5> control_ports{};
  std::atomic<float> echo_input_gain{1.0F};
  std::atomic<float> echo_output_gain{1.0F};

  void service_instance();
};
