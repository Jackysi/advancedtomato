#ifndef _UTIME_H
#define _UTIME_H

#ifdef __cplusplus
extern "C" {
#endif

struct utimbuf {
  time_t actime;
  time_t modtime;
};


#ifdef __cplusplus
}
#endif

#endif /* _UTIME_H */
