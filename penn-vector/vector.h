#ifndef VECTOR_H_
#define VECTOR_H_

/*!
 * The idea is that we have T* which is a pointer to the beginning of
 * heap allocated array (or the pointer is NULL). A null pointer is treated as a
 * 0 length, 0 capacity vector.
 *
 * To store the metadata (capacity, length, element destructor function), we
 * have a vector_info_st struct that is implictly stored before the beginning of
 * the elements. If we did something like:
 *
 * vector(int) vec = vector_new(int, 3, NULL);
 * vector_push(&vec, 1);
 *
 * We would get following stored in memory:
 *
 *             +---+
 *         vec | | |
 *             +-+-+
 *               |
 *               |
 *               V    // pointer to heap
 * +------------+---+---+---+
 * | cap = 3    |   |   |   |
 * | len = 1    | 1 | ? | ? |
 * | dtor= NULL |   |   |   |
 * +------------+---+---+---+
 *
 *
 * This means we can index directly into the vector using square brackets, [].
 * Other than square brackets, almost all functions take in a pointer to a
 * vector so that the vector can be modified
 *
 * To make sure that the ele_dtor can work on any type T, the destructor
 * function takes a pointer to an element in the vector.
 *
 * int main() {
 *   // declares an empty vector with 0 cap and no dtor function
 *   vector(int) vec = NULL;
 *
 *   // could also do this to specify an initial capacity
 *   // and a destroy function (NULL if no destroy function)
 *   // vector(int) vec = vector_new(int, 10, NULL);
 *
 *   vector_push(&vec, 3);
 *   vector_push(&vec, 5);
 *
 *   for (size_t i = 0; i < vector_len(&vec); i++) {
 *      printf("%d\n", vec[i]);
 *   }
 *
 *   vector_free(&vec);
 * }
 *
 */

#include <stdbool.h>
#include <stdlib.h>  // malloc, realloc, free
#include "./panic.h"

// the destroy function takes in a pointer
// to an element of the vector.
typedef void (*destroy_fn)(void*);

typedef struct vector_info_st {
  size_t len;
  size_t capacity;
  destroy_fn ele_dtor;
} vector_info;

#define vector(T) T*

// Synopsis:
//  vector_info* get_vector_header(vector(T)* vec);
//
// args:
//  Given a pointer to a vector, returns a pointer to
//  that vector's metadata which contain the stored
//  capacity, length and element destructor.
//
// Given to you:
#define get_vector_header(vec)                                      \
  ({                                                                \
    typeof(vec) __impl_gvh_vec = (vec);                             \
    if (__impl_gvh_vec == NULL) {                                   \
      panic("NULL arg to get_vector_header\n");                     \
    }                                                               \
    *__impl_gvh_vec ? ((vector_info*)(*__impl_gvh_vec)) - 1 : NULL; \
  })

// Synopsis:
//   vector(T) vector_new(T, size_t initial_capacity, destroy_fn
//   element_destroy_fn);
//
// Description:
//
// This is a "factory" function that allocates a new empty vector
// of the type specified by _T_, the capcaity specified by _initial_capacity_,
// the destructor function to destroy elements in the vector.
// The destructor is run on specific elements in the vector when the element is
// removed or the non-empty vector is de-allocated.
//
// args:
// - T: this is not a real parameter, rather, it specifies the type of the
//      vector being created.
// - init_capacity: the initial capacity of the vector
// - dtor: the element destroy fn
//
// returns:
// - a newly allocated vector
//
// example:
// vector(int) v = vector_new(int, 10, NULL);
#define vector_alloc_size(capacity, element_size) \
  (sizeof(vector_info) + ((capacity) * (element_size)))

#define vector_data_from_header(T, header) ((T*)((header) + 1))

#define vector_next_capacity(capacity) ((capacity) == 0 ? 1 : (capacity)*2)

#define vector_new(T, init_capacity, dtor)                               \
  ({                                                                     \
    size_t pv_capacity = (init_capacity);                                \
    vector_info* pv_header =                                             \
        (vector_info*)malloc(vector_alloc_size(pv_capacity, sizeof(T))); \
    if (pv_header == NULL) {                                             \
      panic("vector_new: malloc failed\n");                              \
    }                                                                    \
    pv_header->len = 0;                                                  \
    pv_header->capacity = pv_capacity;                                   \
    pv_header->ele_dtor = (dtor);                                        \
    vector_data_from_header(T, pv_header);                               \
  })

#define vector_len(self)                              \
  ({                                                  \
    vector_info* pv_header = get_vector_header(self); \
    pv_header == NULL ? 0U : pv_header->len;          \
  })

#define vector_capacity(self)                         \
  ({                                                  \
    vector_info* pv_header = get_vector_header(self); \
    pv_header == NULL ? 0U : pv_header->capacity;     \
  })

#define vector_element_size(self) (sizeof(**(self)))

#define vector_resize(self, n)                                             \
  ({                                                                       \
    typeof(self) vr_vec = (self);                                          \
    size_t vr_new_capacity = (n);                                          \
    size_t vr_old_capacity = vector_capacity(vr_vec);                      \
                                                                           \
    if (vr_new_capacity > vr_old_capacity) {                               \
      vector_info* vr_old_header = get_vector_header(vr_vec);              \
      size_t vr_bytes =                                                    \
          vector_alloc_size(vr_new_capacity, vector_element_size(vr_vec)); \
      vector_info* vr_new_header = NULL;                                   \
                                                                           \
      if (vr_old_header == NULL) {                                         \
        vr_new_header = (vector_info*)malloc(vr_bytes);                    \
        if (vr_new_header != NULL) {                                       \
          vr_new_header->len = 0;                                          \
          vr_new_header->ele_dtor = NULL;                                  \
        }                                                                  \
      } else {                                                             \
        vr_new_header = (vector_info*)realloc(vr_old_header, vr_bytes);    \
      }                                                                    \
                                                                           \
      if (vr_new_header == NULL) {                                         \
        panic("vector_resize: allocation failed\n");                       \
      }                                                                    \
                                                                           \
      vr_new_header->capacity = vr_new_capacity;                           \
      *vr_vec = (typeof(*vr_vec))(vr_new_header + 1);                      \
    }                                                                      \
                                                                           \
    ((void)0);                                                             \
  })

#define vector_get(self, index)                   \
  ({                                              \
    typeof(self) pv_vec = (self);                 \
    size_t pv_index = (index);                    \
    if (pv_index >= vector_len(pv_vec)) {         \
      panic("vector_get: index out of bounds\n"); \
    }                                             \
    (*pv_vec)[pv_index];                          \
  })

#define vector_set(self, index, ...)                    \
  ({                                                    \
    typeof(self) pv_vec = (self);                       \
    size_t pv_index = (index);                          \
    if (pv_index >= vector_len(pv_vec)) {               \
      panic("vector_set: index out of bounds\n");       \
    }                                                   \
                                                        \
    vector_info* pv_header = get_vector_header(pv_vec); \
    if (pv_header->ele_dtor != NULL) {                  \
      pv_header->ele_dtor(&(*pv_vec)[pv_index]);        \
    }                                                   \
                                                        \
    (*pv_vec)[pv_index] = (__VA_ARGS__);                \
    ((void)0);                                          \
  })

#define vector_push(self, ...)                                  \
  ({                                                            \
    typeof(self) vp_vec = (self);                               \
    if (vector_len(vp_vec) == vector_capacity(vp_vec)) {        \
      size_t vp_capacity = vector_capacity(vp_vec);             \
      vector_resize(vp_vec, vector_next_capacity(vp_capacity)); \
    }                                                           \
                                                                \
    vector_info* vp_header = get_vector_header(vp_vec);         \
    (*vp_vec)[vp_header->len] = (__VA_ARGS__);                  \
    vp_header->len++;                                           \
    ((void)0);                                                  \
  })

#define vector_pop(self)                                 \
  ({                                                     \
    typeof(self) pv_vec = (self);                        \
    vector_info* pv_header = get_vector_header(pv_vec);  \
    bool pv_removed = false;                             \
                                                         \
    if (pv_header != NULL && pv_header->len > 0) {       \
      pv_header->len--;                                  \
      if (pv_header->ele_dtor != NULL) {                 \
        pv_header->ele_dtor(&(*pv_vec)[pv_header->len]); \
      }                                                  \
      pv_removed = true;                                 \
    }                                                    \
                                                         \
    pv_removed;                                          \
  })

#define vector_insert(vec, index, ...)                                     \
  ({                                                                       \
    typeof(vec) vi_vec = (vec);                                            \
    size_t vi_index = (index);                                             \
    size_t vi_len = vector_len(vi_vec);                                    \
                                                                           \
    if (vi_index > vi_len) {                                               \
      panic("vector_insert: index out of bounds\n");                       \
    }                                                                      \
                                                                           \
    if (vi_len == vector_capacity(vi_vec)) {                               \
      size_t vi_capacity = vector_capacity(vi_vec);                        \
      vector_resize(vi_vec, vector_next_capacity(vi_capacity));            \
    }                                                                      \
                                                                           \
    vector_info* vi_header = get_vector_header(vi_vec);                    \
    for (size_t vi_curr = vi_header->len; vi_curr > vi_index; vi_curr--) { \
      (*vi_vec)[vi_curr] = (*vi_vec)[vi_curr - 1];                         \
    }                                                                      \
                                                                           \
    (*vi_vec)[vi_index] = (__VA_ARGS__);                                   \
    vi_header->len++;                                                      \
    ((void)0);                                                             \
  })

#define vector_erase(vec, index)                                               \
  ({                                                                           \
    typeof(vec) pv_vec = (vec);                                                \
    size_t pv_index = (index);                                                 \
                                                                               \
    if (pv_index >= vector_len(pv_vec)) {                                      \
      panic("vector_erase: index out of bounds\n");                            \
    }                                                                          \
                                                                               \
    vector_info* pv_header = get_vector_header(pv_vec);                        \
    if (pv_header->ele_dtor != NULL) {                                         \
      pv_header->ele_dtor(&(*pv_vec)[pv_index]);                               \
    }                                                                          \
                                                                               \
    for (size_t pv_curr = pv_index; pv_curr + 1 < pv_header->len; pv_curr++) { \
      (*pv_vec)[pv_curr] = (*pv_vec)[pv_curr + 1];                             \
    }                                                                          \
                                                                               \
    pv_header->len--;                                                          \
    ((void)0);                                                                 \
  })

#define vector_free(self)                                                  \
  ({                                                                       \
    typeof(self) pv_vec = (self);                                          \
    vector_info* pv_header = get_vector_header(pv_vec);                    \
                                                                           \
    if (pv_header != NULL) {                                               \
      if (pv_header->ele_dtor != NULL) {                                   \
        for (size_t pv_index = 0; pv_index < pv_header->len; pv_index++) { \
          pv_header->ele_dtor(&(*pv_vec)[pv_index]);                       \
        }                                                                  \
      }                                                                    \
                                                                           \
      free(pv_header);                                                     \
      *pv_vec = NULL;                                                      \
    }                                                                      \
                                                                           \
    ((void)0);                                                             \
  })

#endif  // VECTOR_H_
