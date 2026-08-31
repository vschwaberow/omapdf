#pragma once

#include <QMetaObject>
#include <QObject>
#include <utility>

class ScopedConnection {
public:
  ScopedConnection() = default;
  explicit ScopedConnection(QMetaObject::Connection connection)
      : m_connection(std::move(connection)) {}
  ~ScopedConnection() { reset(); }

  ScopedConnection(const ScopedConnection &) = delete;
  ScopedConnection &operator=(const ScopedConnection &) = delete;

  ScopedConnection(ScopedConnection &&other) noexcept
      : m_connection(std::exchange(other.m_connection, {})) {}
  ScopedConnection &operator=(ScopedConnection &&other) noexcept {
    if (this != &other) {
      reset();
      m_connection = std::exchange(other.m_connection, {});
    }
    return *this;
  }

  void reset(QMetaObject::Connection connection = {}) {
    if (m_connection) {
      QObject::disconnect(m_connection);
    }
    m_connection = std::move(connection);
  }

  [[nodiscard]] explicit operator bool() const {
    return static_cast<bool>(m_connection);
  }

private:
  QMetaObject::Connection m_connection;
};
