#pragma once

#define TG_CONCAT_INTERNAL(x, y) x##y
#define TG_CONCAT(x, y) TG_CONCAT_INTERNAL(x, y)

#define TG_STRING_INTERNAL(x) #x
#define TG_STRING(x) TG_STRING_INTERNAL(x)

#define TG_DELETE_COPY(class_name)         \
   class_name(const class_name&) = delete; \
   class_name& operator=(const class_name&) = delete;

#define TG_DELETE_MOVE(class_name)             \
   class_name(class_name&&) noexcept = delete; \
   class_name& operator=(class_name&&) noexcept = delete;

#define TG_DELETE_ALL(class_name) \
   TG_DELETE_COPY(class_name)     \
   TG_DELETE_MOVE(class_name)

#define TG_NUMBER_LIST 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0
#define TG_NUM_ARGS_INTERNAL_INTERNAL(arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, N, ...) N
#define TG_NUM_ARGS_INTERNAL(...) TG_NUM_ARGS_INTERNAL_INTERNAL(__VA_ARGS__)
#define TG_NUM_ARGS(...) TG_NUM_ARGS_INTERNAL(__VA_ARGS__ __VA_OPT__(, ) TG_NUMBER_LIST)

#define TG_FOR_EACH_N0(CALLBACK)
#define TG_FOR_EACH_N1(CALLBACK, arg0) CALLBACK(0, arg0)
#define TG_FOR_EACH_N2(CALLBACK, arg0, arg1) CALLBACK(0, arg0), CALLBACK(1, arg1)
#define TG_FOR_EACH_N3(CALLBACK, arg0, arg1, arg2) CALLBACK(0, arg0), CALLBACK(1, arg1), CALLBACK(2, arg2)
#define TG_FOR_EACH_N4(CALLBACK, arg0, arg1, arg2, arg3) CALLBACK(0, arg0), CALLBACK(1, arg1), CALLBACK(2, arg2), CALLBACK(3, arg3)

#define TG_FOR_EACH(CALLBACK, ...) TG_CONCAT(TG_FOR_EACH_N, TG_NUM_ARGS(__VA_ARGS__))(CALLBACK __VA_OPT__(, ) __VA_ARGS__)