#include "erlang/erl_nif.h"
#include "uthash.h"

#include <stdio.h>

#define TRACE printf

typedef struct {
  int id;
  ErlNifEnv *env;
  ErlNifMutex *mutex;
  ErlNifCond *cond;
  ERL_NIF_TERM msg;

  int complete;
  ERL_NIF_TERM result;

  UT_hash_handle hh;
} ErlCall;

static ErlNifPid server;
static int id = 0;
static ErlNifMutex *callsMutex;
static ErlCall *calls;

static ErlCall *CreateCall(ERL_NIF_TERM fun, ERL_NIF_TERM args) {
  enif_mutex_lock(callsMutex);

  ErlCall *erlCall = (ErlCall *)malloc(sizeof(ErlCall));
  erlCall->id = id++;
  erlCall->env = enif_alloc_env();
  ERL_NIF_TERM msgFun = enif_make_copy(erlCall->env, fun);
  ERL_NIF_TERM msgArgs = enif_make_copy(erlCall->env, args);
  ERL_NIF_TERM msgId = enif_make_int(erlCall->env, erlCall->id);
  erlCall->msg = enif_make_tuple3(erlCall->env, msgId, msgFun, msgArgs);
  erlCall->cond = enif_cond_create("erlcl_cond");
  erlCall->mutex = enif_mutex_create("erlcl_mutex");
  erlCall->complete = 0;

  HASH_ADD_INT(calls, id, erlCall);

  enif_mutex_unlock(callsMutex);

  return erlCall;
}

static ErlCall *FindCall(int id) {
  enif_mutex_lock(callsMutex);

  ErlCall *erlCall;
  HASH_FIND_INT(calls, &id, erlCall);

  enif_mutex_unlock(callsMutex);

  return erlCall;
}

static void DestroyCall(ErlCall *erlCall) {
  enif_mutex_lock(callsMutex);

  HASH_DEL(calls, erlCall);
  enif_clear_env(erlCall->env);
  enif_free_env(erlCall->env);
  enif_mutex_destroy(erlCall->mutex);
  enif_cond_destroy(erlCall->cond);
  free(erlCall);

  enif_mutex_unlock(callsMutex);
}

static ERL_NIF_TERM ErlangCall(ErlNifEnv *env, ERL_NIF_TERM fun, ERL_NIF_TERM args) {
  ErlCall *erlCall = CreateCall(fun, args);

  enif_mutex_lock(erlCall->mutex);
  enif_send(env, &server, erlCall->env, erlCall->msg);
  while(!erlCall->complete) {
    enif_cond_wait(erlCall->cond, erlCall->mutex);
  }
  enif_mutex_unlock(erlCall->mutex);

  ERL_NIF_TERM result = enif_make_copy(env, erlCall->result);
  DestroyCall(erlCall);

  return result;
}

static ERL_NIF_TERM SetPid(ErlNifEnv *env,
    int argc,
    const ERL_NIF_TERM argv[]) {
  TRACE("SetPid\n");
  if(enif_get_local_pid(env, argv[0], &server)) {
    return enif_make_atom(env, "ok");
  } else {
    return enif_make_badarg(env);
  }
}

static ERL_NIF_TERM GetPid(ErlNifEnv *env,
    int argc,
    const ERL_NIF_TERM argv[]) {
  TRACE("GetPid\n");
  return enif_make_pid(env, &server);
}

static ERL_NIF_TERM Respond(ErlNifEnv *env,
    int argc,
    const ERL_NIF_TERM argv[]) {
  TRACE("Respond\n");
  int id;

  if(enif_get_int(env, argv[0], &id)) {
    ErlCall *erlCall = FindCall(id);

    if(erlCall) {
      enif_mutex_lock(erlCall->mutex);
      erlCall->result = argv[1];
      erlCall->complete = 1;
      enif_cond_broadcast(erlCall->cond);
      enif_mutex_unlock(erlCall->mutex);

      return enif_make_atom(env, "ok");
    } else {
      return enif_make_badarg(env);
    }
  } else {
    return enif_make_badarg(env);
  }
}

typedef struct {
  ErlNifEnv *env;
  ERL_NIF_TERM fun;
  ERL_NIF_TERM args;
} TCall;

static ErlNifTid tid;
static void *TestCall(void *ptr) {
  TCall *call = (TCall *)ptr;
  ERL_NIF_TERM result = ErlangCall(call->env, call->fun, call->args);
  int length;
  char *buffer;
  enif_get_atom_length(call->env, result, &length, ERL_NIF_LATIN1);
  buffer = (char *)malloc((length + 1) * sizeof(char));
  enif_get_atom(call->env, result, buffer, length + 1, ERL_NIF_LATIN1);
  printf("THIS IS YOUR RESULT(%d)! %s\n", length, buffer);

  return NULL;
}

static ERL_NIF_TERM MakeTestCall(ErlNifEnv *env,
    int argc,
    const ERL_NIF_TERM argv[]) {
  TCall *call = (TCall *)malloc(sizeof(TCall));
  call->env = env;
  call->fun = argv[0];
  call->args = argv[1];
  enif_thread_create("my_thread", &tid, TestCall, (void *)call, NULL);

  return enif_make_atom(env, "ok");
}

static ErlNifFunc nif_funcs[] = {
  {"set_pid", 1, SetPid},
  {"get_pid", 0, GetPid},
  {"respond", 2, Respond},
  {"make_test_call", 2, MakeTestCall}
};

static int Load(
    ErlNifEnv *env,
    void** priv_data,
    ERL_NIF_TERM load_info) {
  callsMutex = enif_mutex_create("erlcl_calls");
  return 0;
};

ERL_NIF_INIT(erlcl_nif, nif_funcs, Load, NULL, NULL, NULL);
