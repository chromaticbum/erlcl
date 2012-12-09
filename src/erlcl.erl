-module(erlcl).

-export([
  start/0
  ]).

start() ->
  application:start(erlcl).
