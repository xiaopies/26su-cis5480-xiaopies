/* CIS5480 Assignment 0
 * Author: Xiaoshen Ma
 * Purpose: Implements a generic dynamically resizable vector of pointers.
 */

#include "./Vec.h"
#include <stdlib.h>
#include "./panic.h"

static void destroy_element(Vec* self, size_t index) {
  if (self->ele_dtor_fn != NULL) {
    self->ele_dtor_fn(self->data[index]);
  }
}

static size_t next_capacity(size_t current_capacity) {
  if (current_capacity == 0) {
    return 1;
  }

  return current_capacity * 2;
}

static void ensure_capacity_for_one_more(Vec* self) {
  if (self->length < self->capacity) {
    return;
  }

  vec_resize(self, next_capacity(self->capacity));
}

Vec vec_new(size_t initial_capacity, ptr_dtor_fn ele_dtor_fn) {
  Vec new_vec = {
      .data = NULL,
      .length = 0,
      .capacity = initial_capacity,
      .ele_dtor_fn = ele_dtor_fn,
  };

  if (initial_capacity == 0) {
    return new_vec;
  }

  new_vec.data = malloc(initial_capacity * sizeof(ptr_t));
  if (new_vec.data == NULL) {
    panic("vec_new: malloc failed");
  }

  return new_vec;
}

ptr_t vec_get(Vec* self, size_t index) {
  if (index >= self->length) {
    panic("vec_get: index out of bounds");
  }

  return self->data[index];
}

void vec_set(Vec* self, size_t index, ptr_t new_ele) {
  if (index >= self->length) {
    panic("vec_set: index out of bounds");
  }

  if (self->data[index] != new_ele) {
    destroy_element(self, index);
  }

  self->data[index] = new_ele;
}

void vec_push_back(Vec* self, ptr_t new_ele) {
  ensure_capacity_for_one_more(self);

  self->data[self->length] = new_ele;
  self->length++;
}

bool vec_pop_back(Vec* self) {
  if (vec_is_empty(self)) {
    return false;
  }

  self->length--;
  destroy_element(self, self->length);
  self->data[self->length] = NULL;

  return true;
}

void vec_insert(Vec* self, size_t index, ptr_t new_ele) {
  if (index > self->length) {
    panic("vec_insert: index out of bounds");
  }

  ensure_capacity_for_one_more(self);

  for (size_t curr_index = self->length; curr_index > index; curr_index--) {
    self->data[curr_index] = self->data[curr_index - 1];
  }

  self->data[index] = new_ele;
  self->length++;
}

void vec_erase(Vec* self, size_t index) {
  if (index >= self->length) {
    panic("vec_erase: index out of bounds");
  }

  destroy_element(self, index);

  for (size_t curr_index = index; curr_index + 1 < self->length; curr_index++) {
    self->data[curr_index] = self->data[curr_index + 1];
  }

  self->length--;
  self->data[self->length] = NULL;
}

void vec_resize(Vec* self, size_t new_capacity) {
  if (new_capacity <= self->length) {
    return;
  }

  ptr_t* new_data = realloc(self->data, new_capacity * sizeof(ptr_t));
  if (new_data == NULL) {
    panic("vec_resize: realloc failed");
  }

  self->data = new_data;
  self->capacity = new_capacity;
}

void vec_clear(Vec* self) {
  for (size_t curr_index = 0; curr_index < self->length; curr_index++) {
    destroy_element(self, curr_index);
    self->data[curr_index] = NULL;
  }

  self->length = 0;
}

void vec_destroy(Vec* self) {
  vec_clear(self);
  free(self->data);

  self->data = NULL;
  self->length = 0;
  self->capacity = 0;
  self->ele_dtor_fn = NULL;
}
