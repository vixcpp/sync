/**
 *
 *  @file fake_transport.hpp
 *  @author Gaspard Kirira
 *
 *  Vix.cpp - Sync examples
 *
 */
#ifndef VIX_EXAMPLES_SYNC_FAKE_TRANSPORT_HPP
#define VIX_EXAMPLES_SYNC_FAKE_TRANSPORT_HPP

#include <string>
#include <unordered_map>
#include <utility>

#include <vix/sync/engine/SyncWorker.hpp>

namespace vix::sync::engine
{
  class FakeTransport final : public ISyncTransport
  {
  public:
    struct Rule
    {
      bool ok{true};
      bool retryable{true};
      std::string error{"simulated failure"};
    };

    void setDefault(Rule r) { def_ = std::move(r); }

    void setRuleForKind(std::string kind, Rule r)
    {
      by_kind_[std::move(kind)] = std::move(r);
    }

    void setRuleForTarget(std::string target, Rule r)
    {
      by_target_[std::move(target)] = std::move(r);
    }

    std::size_t callCount() const noexcept { return calls_; }

    SendResult send(const vix::sync::Operation &op) override
    {
      ++calls_;

      if (auto it = by_target_.find(op.target); it != by_target_.end())
        return toResult(it->second);

      if (auto it = by_kind_.find(op.kind); it != by_kind_.end())
        return toResult(it->second);

      return toResult(def_);
    }

  private:
    static SendResult toResult(const Rule &r)
    {
      SendResult res;
      res.ok = r.ok;
      res.retryable = r.retryable;
      res.error = r.ok ? "" : r.error;
      return res;
    }

  private:
    Rule def_{};
    std::unordered_map<std::string, Rule> by_kind_;
    std::unordered_map<std::string, Rule> by_target_;
    std::size_t calls_{0};
  };

} // namespace vix::sync::engine

#endif
