#!/bin/bash

make release

./build/release/ms res/cubic_torus.obj_7.obj _ 25
./build/release/ms res/cube_hole.obj_7.obj _ 25
./build/release/ms res/gear_hole.obj_7.obj _ 10