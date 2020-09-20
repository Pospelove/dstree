**dstree** is tree-like structure optimized for serialization. 

 - *Keys are not unique.* Nodes can store keys with equal values.
 - *Serialization is free.* In case of trusted data source and similar endianness no serialization is needed at all, since internal representation of the tree is flat array of bytes.
 - *Find is O(log n)*, insert/erase is O(n) where n is number of child nodes of the node operation performed on.

```c++
void foo(const uint8_t* data, size_t length) {
  dstree t = dstree::deserialize(data, length, owning_mode::non_owning);
}
```
Structure supports `insert`, `erase`,  `find`, `size` and other operations (see dstree.hpp).

```c++
void bar() {
  dstree t;
  t.insert(2.015);
  t.insert(2015LL);
  t.insert("2015");
  t.find(2.015).insert(2.020);
}
```
dstree has been tested on:
- MSVC
