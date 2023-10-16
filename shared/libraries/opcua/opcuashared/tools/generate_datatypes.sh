#!/bin/bash

OPEN62541_ROOT="C:/DXEProjects/Apps/Blueberry/build/x64/msvc-19/full/external/bbopen62541/build/open62541-src"

python $OPEN62541_ROOT/tools/generate_datatypes.py \
	--namespace="1:https://blueberrydaq.com/ua" \
	--type-csv=schema/BB.NodeIds.csv \
	--type-bsd=schema/BB.Opc.Ua.Types.bsd \
	--no-builtin \
	bb
