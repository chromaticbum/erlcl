all: compile

compile: rebar_compile
	
rebar_compile:
	./rebar compile

clean:
	./rebar clean

console: compile
	erl -pa ../erlcl -pa ebin/ -s erlcl

test: eunit

eunit: compile
	./rebar -C rebar.tests.config eunit skip_deps=true
