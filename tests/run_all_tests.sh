#!/bin/bash

arr=($(ls | grep \.x$))

for i in "${arr[@]}"; do
	./$i
done
