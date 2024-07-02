@echo off
if not exist out (
	mkdir out
)
pushd out
cl ..\main.c ..\http_parser.c
popd
