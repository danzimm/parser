
#ifndef __properties_H
#define __properties_H

#define proprog(t, n, g) \
protected: \
  t _ ## n; \
public: \
  t n() const { t out = _ ## n; { g }; return _ ## n; }

#define propsg(t, n, s, g) \
protected: \
  t _ ## n; \
public: \
  t& n() { t& out = _ ## n; { g }; return out; } \
  t n() const { t out = _ ## n; { g }; return out; } \
  auto& n(t val) { { s }; _ ## n = val; return (*this); }

#define propg(t, n, g) \
protected: \
  t _ ## n; \
public: \
  t& n() { t& out = _ ## n; { g }; return out; } \
  t n() const { t out = _ ## n; { g }; return out; } \
  auto& n(t val) { _ ## n = val; return (*this); }

#define props(t, n, s) \
protected: \
  t _ ## n; \
public: \
  t& n() { return _ ## n; } \
  t n() const { return _ ## n; } \
  auto& n(t val) { { s }; _ ## n = val; return (*this); }

#define propro(t, n) \
protected: \
  t _ ## n; \
public: \
  t n() const { return _ ## n; }

#define prop(t, n) \
protected: \
  t _ ## n; \
public: \
  t& n() { return _ ## n; } \
  t n() const { return _ ## n; } \
  auto& n(t val) { _ ## n = val; return (*this); }

#endif

