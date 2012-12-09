-module(erlcl_nif).
-on_load(init/0).

-export([
  init/0,
  set_pid/1,
  get_pid/0,
  make_test_call/2,
  respond/2
  ]).

init() ->
  case code:priv_dir(erlcl) of
    {error, Reason} -> {error, Reason};
    Filename ->
      erlang:load_nif(filename:join([Filename, "erlcl_drv"]), 0)
  end.

set_pid(_Pid) ->
  error(not_loaded).

get_pid() ->
  error(not_loaded).

make_test_call(_Fun, _Args) ->
  error(not_loaded).

respond(_Id, _Result) ->
  error(not_loaded).

-ifdef(TEST).
-include_lib("eunit/include/eunit.hrl").

-endif.
