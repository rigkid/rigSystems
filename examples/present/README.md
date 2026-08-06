# Present

![preview](img/preview.png)

A turning dial: four arms and their moons hang off one
root, `SHierarchy` folds the parent chain into every world matrix, and
`SShapeRendering` presents the shapes.

Update writes seven transforms in total: the root gets the window centre and a
fit scale, the four arms and two moon joints each get an angle. Rings, ticks, rays and discs are
plain data — the hierarchy carries them.

```bash
cmake -S . -B build
cmake --build build --target present
./build/bin/present/present
```

Set `RIGKIT_DIR` if the host is not four levels above this folder.
