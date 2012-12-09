
-module(erlcl_sup).

-behaviour(supervisor).

%% API
-export([start_link/0]).

%% Supervisor callbacks
-export([init/1]).

-define(CHILD(I, Type), {I, {I, start_link, []}, permanent, 5000, Type, [I]}).

%% ===================================================================
%% API functions
%% ===================================================================

start_link() ->
  supervisor:start_link({local, ?MODULE}, ?MODULE, []).

%% ===================================================================
%% Supervisor callbacks
%% ===================================================================

init([]) ->
  ErlclSrvSpec = {erlcl_srv,
                  {erlcl_srv, start_link, []},
                  permanent, 5000, worker, [erlcl_srv]},
  {ok, { {one_for_one, 5, 10}, [ErlclSrvSpec]} }.

