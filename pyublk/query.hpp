#pragma once

namespace ublk {

class query {
public:
  query() = default;

  query(query const &) = default;
  query &operator=(query const &) = default;

  query(query &&) = default;
  query &operator=(query &&) = default;

protected:
  ~query() = default;

public:
  void set_err(int err) noexcept { err_ = err; }
  int err() const noexcept { return err_; }

private:
  int err_{0};
};

} // namespace ublk
