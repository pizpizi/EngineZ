#pragma once

#include <cstdint>
#include <unordered_map>

template <typename T, typename Handle = uint32_t> class ObjectRegistry {
  public:
    Handle put(T* obj) {
        counter++;
        data.emplace(counter, obj);
        return counter;
    }
    T* get(Handle id) {
        auto it = data.find(id);

        if (it == data.end()) {
            return nullptr;
        }

        return it->second;
    }
    T* takeOut(Handle id) {
        auto it = data.find(id);

        if (it == data.end()) {
            return nullptr;
        }

        auto obj = it->second;
        data.erase(it);
        return obj;
    }

    std::unordered_map<Handle, T*>& getData() {
        return data;
    }

  private:
    Handle counter = 0;

    std::unordered_map<Handle, T*> data;
};