/**
 *
 *  @file RetryPolicy.hpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2025, Gaspard Kirira.
 *  All rights reserved.
 *  https://github.com/vixcpp/vix
 *
 *  Use of this source code is governed by a MIT license
 *  that can be found in the License file.
 *
 *  Vix.cpp
 *
 */
#ifndef VIX_SYNC_RETRY_POLICY_HPP
#define VIX_SYNC_RETRY_POLICY_HPP

#include <algorithm>
#include <cstdint>

namespace vix::sync
{
  /**
   * @brief Exponential backoff retry policy.
   *
   * RetryPolicy defines how failed operations are retried over time.
   * It supports:
   * - a maximum number of attempts
   * - exponential backoff based on a base delay and factor
   * - clamping to a maximum delay
   * - optional jitter (documented here, applied by caller if needed)
   *
   * The policy itself is deterministic and side-effect free, making it
   * suitable for durable systems where retry timing must be recomputable
   * during recovery.
   */
  struct RetryPolicy
  {
    /**
     * @brief Maximum number of retry attempts.
     *
     * Once attempt >= max_attempts, no further retries are allowed.
     */
    std::uint32_t max_attempts{8};

    /**
     * @brief Base delay in milliseconds for the first retry.
     */
    std::int64_t base_delay_ms{500}; // 0.5s

    /**
     * @brief Maximum delay in milliseconds between retries.
     */
    std::int64_t max_delay_ms{30'000}; // 30s

    /**
     * @brief Exponential backoff factor.
     *
     * Delay grows as: base_delay_ms * (factor ^ attempt).
     */
    double factor{2.0};

    /**
     * @brief Jitter ratio applied by higher-level logic.
     *
     * Expressed as a fraction:
     * - 0.0  = no jitter
     * - 0.2  = +/-20% randomization window
     *
     * @note This policy does not apply jitter directly. It exposes the
     * parameter so callers (e.g. Outbox or Engine) can apply randomness
     * in a controlled manner.
     */
    double jitter_ratio{0.2};

    /**
     * @brief Check whether another retry is allowed.
     *
     * @param attempt Current attempt count (0-based).
     * @return true if another retry may be scheduled.
     */
    bool can_retry(std::uint32_t attempt) const noexcept
    {
      return attempt < max_attempts;
    }

    /**
     * @brief Compute the retry delay for a given attempt.
     *
     * The delay grows exponentially and is clamped between
     * base_delay_ms and max_delay_ms.
     *
     * @param attempt Current attempt count (0-based).
     * @return Delay in milliseconds before the next retry.
     */
    std::int64_t compute_delay_ms(std::uint32_t attempt) const noexcept
    {
      // attempt: 0,1,2,... (0 => base)
      double d = static_cast<double>(base_delay_ms);
      for (std::uint32_t i = 0; i < attempt; ++i)
        d *= factor;

      auto delay = static_cast<std::int64_t>(d);
      return std::clamp(delay, base_delay_ms, max_delay_ms);
    }
  };

} // namespace vix::sync

#endif // VIX_SYNC_RETRY_POLICY_HPP
