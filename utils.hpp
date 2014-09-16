
#include <functional>

#ifndef __util_H
#define __util_H

#define create_object(cls, initargs, impl) \
  cascade<cls *>(new cls initargs, [](cls *obj) { \
    impl; \
  })

template<typename T>
static inline T cascade(T obj, std::function<void(T)> cb) {
  cb(obj);
  return obj;
}

#endif

