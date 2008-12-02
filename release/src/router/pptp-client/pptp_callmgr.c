/* pptp_callmgr.c ... Call manager for PPTP connections.
 *                    Handles TCP port 1723 protocol.
 *                    C. Scott Ananian <cananian@alumni.princeton.edu>
 *
 * $Id: pptp_callmgr.c,v 1.2 2002/08/23 01:23:28 honor Exp $
 */
#include <signal.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <setjmp.h>
#include <stdio.h>
#include <errno.h>
#include "pptp_callmgr.h"
#include "pptp_ctrl.h"
#include "pptp_msg.h"
#include "dirutil.h"
#include "vector.h"
#include "util.h"
#include <fcntl.h>

int open_inetsock(struct in_addr inetaddr);
int open_unixsock(struct in_addr inetaddr);
void close_inetsock(int fd, struct in_addr inetaddr);
void close_unixsock(int fd, struct in_addr inetaddr);

sigjmp_buf env;

void sighandler(int sig) {
    siglongjmp (env, 1);
}

void do_nothing(int sig) {
    /* do nothing signal handler */
}

struct local_callinfo {
  int unix_sock;
  pid_t pid[2];
};

struct local_conninfo {
  VECTOR * call_list;
  fd_set * call_set;
};

/* Connection callback */
void conn_callback(PPTP_CONN *conn, enum conn_state state) {

  switch(state) {
  case CONN_OPEN_FAIL:
  case CONN_CLOSE_DONE:
    /* get outta here */
    siglongjmp(env, 1);
    break;
  default:
    log("Unhandled connection callback state [%d].", (int) state);
    break;
  }
}
/* Call callback */
void call_callback(PPTP_CONN *conn, PPTP_CALL *call, enum call_state state) {
  struct local_callinfo *lci;
  struct local_conninfo *conninfo;
  u_int16_t call_id[2];

  switch(state) {
  case CALL_OPEN_DONE:
    /* okey dokey.  This means that the call_id and peer_call_id are now
     * valid, so lets send them on to our friends who requested this call.
     */
    lci = pptp_call_closure_get(conn, call); assert(lci != NULL);
    pptp_call_get_ids(conn, call, &call_id[0], &call_id[1]);
    write(lci->unix_sock, &call_id, sizeof(call_id));
    /* Our duty to the fatherland is now complete. */
    break;
  case CALL_OPEN_FAIL:
  case CALL_CLOSE_RQST:
  case CALL_CLOSE_DONE:
    /* don't need to do anything here, except make sure tables are sync'ed */
    log("Closing connection");
    conninfo = pptp_conn_closure_get(conn);
    lci = pptp_call_closure_get(conn, call); 
    assert(lci != NULL && conninfo != NULL);
    if (vector_contains(conninfo->call_list, lci->unix_sock)) {
      vector_remove(conninfo->call_list, lci->unix_sock);
      close(lci->unix_sock);
      FD_CLR(lci->unix_sock, conninfo->call_set);
      if(lci->pid[0]) kill(lci->pid[0], SIGTERM);
      if(lci->pid[1]) kill(lci->pid[1], SIGTERM);
    }
    break;
  default:
    log("Unhandled call callback state [%d].", (int) state);
    break;
  }
}

/* Call Manager */

/* NOTE ABOUT 'VOLATILE':                                              */
/* several variables here get a volatile qualifier to silence warnings */
/* from older (before 3.0) gccs. once the longjmp stuff is removed,    */
/* the volatile qualifiers should be removed as well.                  */ 
int main(int argc, char **argv, char **envp) {
  struct in_addr inetaddr;
  int inet_sock, unix_sock;
  fd_set call_set;
  PPTP_CONN * conn;
  VECTOR * call_list;
  int max_fd=0;
  volatile int first=1;
  int retval;
  int i;
  char * volatile phonenr;

  /* Step 0: Check arguments */
  if (argc < 2) 
    fatal("Usage: %s ip.add.ress.here [--phone <phone number>]", argv[0]);
  phonenr = argc==3 ? argv[2] : NULL;
  if (inet_aton(argv[1], &inetaddr)==0)
    fatal("Invalid IP address: %s", argv[1]);

  /* Step 1: Open sockets. */
  if ((inet_sock = open_inetsock(inetaddr)) < 0)
    fatal("Could not open control connection to %s", argv[1]);
  if ((unix_sock = open_unixsock(inetaddr)) < 0)
    fatal("Could not open unix socket for %s", argv[1]);

  /* Step 1b: FORK and return status to calling process. */
  switch (fork()) {
  case 0: /* child. stick around. */
    break;
  case -1: /* failure.  Fatal. */
    fatal("Could not fork.");
  default: /* Parent. Return status to caller. */
    exit(0);
  }

  /* Step 1c: Clean up unix socket on TERM */
  if (sigsetjmp(env, 1)!=0)
    goto cleanup;

  signal(SIGINT, sighandler);
  signal(SIGTERM, sighandler);

  signal(SIGPIPE, do_nothing);
  signal(SIGUSR1, do_nothing); /* signal state change; wake up accept */

  /* Step 2: Open control connection and register callback */
  if ((conn = pptp_conn_open(inet_sock, 1, NULL/* callback */)) == NULL) {
    close(unix_sock); close(inet_sock); fatal("Could not open connection.");
  }

  FD_ZERO(&call_set);
  max_fd = unix_sock;
  call_list = vector_create();
  { 
    struct local_conninfo *conninfo = malloc(sizeof(*conninfo));
    if (conninfo==NULL) {
      close(unix_sock); close(inet_sock); fatal("No memory.");
    }
    conninfo->call_list = call_list;
    conninfo->call_set  = &call_set;
    pptp_conn_closure_put(conn, conninfo);
  }

  if (sigsetjmp(env, 1)!=0) goto shutdown;

  /* Step 3: Get FD_SETs */
  do {
    int rc;
    fd_set read_set = call_set, write_set;
    FD_ZERO (&write_set);
    FD_SET (unix_sock, &read_set);
    pptp_fd_set(conn, &read_set, &write_set, &max_fd);

    for (; max_fd > 0 ; max_fd--) {
      if (FD_ISSET (max_fd, &read_set) ||
          FD_ISSET (max_fd, &write_set))
        break;
    }

    /* Step 4: Wait on INET or UNIX event */

    if ((rc = select(max_fd+1, &read_set, &write_set, NULL, NULL)) <0)
      /* a signal or somesuch. */
      continue;

    /* Step 5a: Handle INET events */
    pptp_dispatch(conn, &read_set, &write_set);

    /* Step 5b: Handle new connection to UNIX socket */
    if (FD_ISSET(unix_sock, &read_set)) {
      /* New call! */
      struct sockaddr_un from;
      int len = sizeof(from);
      PPTP_CALL * call;
      struct local_callinfo *lci;
      int s;

      /* Accept the socket */
      FD_CLR (unix_sock, &read_set);
      if ((s = accept(unix_sock, (struct sockaddr *) &from, &len))<0) {
	warn("Socket not accepted: %s", strerror(errno));
	goto skip_accept;
      }
      /* Allocate memory for local call information structure. */
      if ((lci = malloc(sizeof(*lci))) == NULL) {
	warn("Out of memory."); close(s); goto skip_accept;
      }
      lci->unix_sock = s;

      /* Give the initiator time to write the PIDs while we open the call */
      call = pptp_call_open(conn, call_callback, phonenr);
      /* Read and store the associated pids */
      read(s, &lci->pid[0], sizeof(lci->pid[0]));
      read(s, &lci->pid[1], sizeof(lci->pid[1]));
      /* associate the local information with the call */
      pptp_call_closure_put(conn, call, (void *) lci);
      /* The rest is done on callback. */
      
      /* Keep alive; wait for close */
      retval = vector_insert(call_list, s, call); assert(retval);
      if (s > max_fd) max_fd = s;
      FD_SET(s, &call_set);
      first = 0;
    }
  skip_accept:
    /* Step 5c: Handle socket close */
    for (i=0; i<max_fd+1; i++)
      if (FD_ISSET(i, &read_set)) {
	/* close it */
	PPTP_CALL * call;
	retval = vector_search(call_list, i, &call);
	if (retval) {
	  struct local_callinfo *lci = pptp_call_closure_get(conn, call);
          log("Closing connection");
	  if(lci->pid[0]) kill(lci->pid[0], SIGTERM);
	  if(lci->pid[1]) kill(lci->pid[1], SIGTERM);
	  free(lci);
	  /* soft shutdown.  Callback will do hard shutdown later */
	  pptp_call_close(conn, call);
	  vector_remove(call_list, i);
	}
	FD_CLR(i, &call_set);
	close(i);
      }
  } while (vector_size(call_list)>0 || first);

shutdown:
  {
    fd_set read_set, write_set;

    /* warn("Shutdown"); */
    /* kill all open calls */
    for (i=0; i<vector_size(call_list); i++) {
      PPTP_CALL *call = vector_get_Nth(call_list, i);
      struct local_callinfo *lci = pptp_call_closure_get(conn, call);
      log("Closing connection");
      pptp_call_close(conn, call);
      if(lci->pid[0]) kill(lci->pid[0], SIGTERM);
      if(lci->pid[1]) kill(lci->pid[1], SIGTERM);
    }
    /* attempt to dispatch these messages */
    FD_ZERO(&read_set);
    FD_ZERO(&write_set);
    pptp_fd_set(conn, &read_set, &write_set, &max_fd);
    FD_ZERO(&read_set);
    pptp_dispatch(conn, &read_set, &write_set);
    if (i>0) sleep(2);
    /* no more open calls.  Close the connection. */
    pptp_conn_close(conn, PPTP_STOP_LOCAL_SHUTDOWN);
    FD_ZERO(&read_set);
    FD_ZERO(&write_set);
    pptp_fd_set(conn, &read_set, &write_set, &max_fd);
    FD_ZERO(&read_set);
    pptp_dispatch(conn, &read_set, &write_set);
    sleep(2);
    /* with extreme prejudice */
    pptp_conn_destroy(conn);
    vector_destroy(call_list);
  }
cleanup:
  close_inetsock(inet_sock, inetaddr);
  close_unixsock(unix_sock, inetaddr);
  return 0;
}

/**
 * Connect socket, but with a timeout.
 * A signal will interrupt this function.
 *
 * param: msec is the timeout in milliseconds
 * return: -1 on error, errno is set
 *         -2 on timeout
 *          0 on successful
 */
int connect_timeout(int socket, struct sockaddr *addr, socklen_t len, int msec) 
{ 
  int oldflags; /* flags to be restored later */ 
  int newflags; /* nonblocking sockopt for socket */ 
  int ret;      /* Result of syscalls */ 
  int value;    /* Value to be returned */ 
 
  /* Make sure the socket is nonblocking */ 
  oldflags = fcntl(socket, F_GETFL, 0); 
  if(oldflags == -1) 
    return -1; 
  newflags = oldflags | O_NONBLOCK; 
  ret = fcntl(socket, F_SETFL, newflags); 
  if(ret == -1) 
    return -1; 
 
  /* Issue the connect request */ 
  ret = connect(socket, addr, len); 
  value = ret; 
 
  if(ret == 0) 
  { 
    /* Connect finished directly */ 
  } 
  else if(errno == EINPROGRESS || errno == EWOULDBLOCK) 
  { 
    fd_set wset; 
    struct timeval tv; 
 
    FD_ZERO(&wset); 
    FD_SET(socket, &wset); 
 
    tv.tv_sec = msec / 1000; 
    tv.tv_usec = 1000*(msec % 1000); 
 
    ret = select(socket+1, NULL, &wset, NULL, &tv); 
 
    if(ret == 1 && FD_ISSET(socket, &wset)) 
    { 
      int optval; 
      socklen_t optlen = sizeof(optval); 
 
      ret = getsockopt(socket, SOL_SOCKET, SO_ERROR, &optval, &optlen); 
      if(ret == -1) 
        return -1; 
 
      if(optval==0) 
        value = 0; 
      else 
        value = -1; 
    } 
    else if(ret == 0) {
	warn("connect timeout for %d seconds",msec / 1000);
        value = -2; /* select timeout */ 
    }
    else {
	warn("select error");
      	value = -1; /* select error */ 
    }
  } 
  else 
    value = -1; 
 
  /* Restore the socket flags */ 
  ret = fcntl(socket, F_SETFL, oldflags); 
  if(ret == -1) 
    perror("restoring socket flags: fcntl"); 
 
  return value; 
} 

int open_inetsock(struct in_addr inetaddr) {
  struct sockaddr_in dest;
  int s;

  dest.sin_family = AF_INET;
  dest.sin_port   = htons(PPTP_PORT);
  dest.sin_addr   = inetaddr;

  if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    warn("socket: %s", strerror(errno));
    return s;
  }
  if (connect_timeout(s, (struct sockaddr *) &dest, sizeof(dest), 15000) < 0) {
    warn("connect: %s", strerror(errno));
    close(s); return -1;
  }
  return s;
}
int open_unixsock(struct in_addr inetaddr) {
  struct sockaddr_un where;
  struct stat st;
  char *dir;
  int s;

  if ((s = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
    warn("socket: %s", strerror(errno));
    return s;
  }

  where.sun_family = AF_UNIX;
  snprintf(where.sun_path, sizeof(where.sun_path), 
	   PPTP_SOCKET_PREFIX "%s", inet_ntoa(inetaddr));

  if (stat(where.sun_path, &st) >= 0) {
    warn("Call manager for %s is already running.", inet_ntoa(inetaddr));
    close(s); return -1;
  }

  /* Make sure path is valid. */
  dir = dirname(where.sun_path);
  if (!make_valid_path(dir, 0770))
    fatal("Could not make path to %s: %s", where.sun_path, strerror(errno));
  free(dir);

  if (bind(s, (struct sockaddr *) &where, sizeof(where)) < 0) {
    warn("bind: %s", strerror(errno));
    close(s); return -1;
  }

  chmod(where.sun_path, 0777);

  listen(s, 127);

  return s;
}
void close_inetsock(int fd, struct in_addr inetaddr) {
  close(fd);
}
void close_unixsock(int fd, struct in_addr inetaddr) {
  struct sockaddr_un where;
  close(fd);
  snprintf(where.sun_path, sizeof(where.sun_path), 
	   PPTP_SOCKET_PREFIX "%s", inet_ntoa(inetaddr));
  unlink(where.sun_path);
}
