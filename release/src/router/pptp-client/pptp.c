/* pptp.c ... client shell to launch call managers, data handlers, and
 *            the pppd from the command line.
 *            C. Scott Ananian <cananian@alumni.princeton.edu>
 *
 * $Id: pptp.c,v 1.2 2002/08/20 07:11:40 honor Exp $
 */

#include <sys/types.h>
#include <sys/socket.h>
#if defined(__FreeBSD__)
#include <libutil.h>
#elif defined(__NetBSD__)
#include <util.h>
#else
#include <pty.h>
#endif
//#ifdef USER_PPP
#include <fcntl.h>
//#endif
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <setjmp.h>
#include <errno.h>
#include <sys/wait.h>
#include <getopt.h>
#include <limits.h>
#include "config.h"
#include "pptp_callmgr.h"
#include "pptp_gre.h"
#include "version.h"
#include "inststr.h"
#include "util.h"
#include "pptp_quirks.h"

#ifndef PPPD_BINARY
#define PPPD_BINARY "pppd"
#endif

struct in_addr get_ip_address(char *name);
int open_callmgr(struct in_addr inetaddr, char *phonenr, int argc,char **argv,char **envp);
void launch_callmgr(struct in_addr inetaddr, char *phonenr, int argc,char **argv,char **envp);
int get_call_id(int sock, pid_t gre, pid_t pppd, 
		 u_int16_t *call_id, u_int16_t *peer_call_id);
void launch_pppd(char *ttydev, int argc, char **argv);


/* TODO: redesign to avoid longjmp/setjmp.  Several variables here
   have a volatile qualifier to silence warnings from gcc < 3.0.
   Remove the volatile qualifiers when longjmp/setjmp are removed. */
/* Create pseudo tty master slave pair and set terminal attributes
   according to TERMP and WINP.  Return handles for both ends in
   AMASTER and ASLAVE, and return the name of the slave end in NAME.  */

/* from uClibc 0.9.9 openpty.c */
int
my_openpty (int *amaster, int *aslave, char *name, struct termios *termp,
         struct winsize *winp)
{
#if 0
#ifdef PATH_MAX
  char _buf[PATH_MAX];
#else
  char _buf[512];
#endif
  char *buf = _buf;
#else
#ifdef PATH_MAX
  char buf[PATH_MAX];
#else
  char buf[512];
#endif
#endif
  int master, slave;

  master = getpt ();
  if (master == -1)
    return -1;

  if (grantpt (master))
    goto fail;

  if (unlockpt (master))
    goto fail;

#if 0
  if (pts_name (master, &buf, sizeof (_buf)))
#else
  if (ptsname_r (master, buf, sizeof buf))
#endif
    goto fail;

  slave = open (buf, O_RDWR | O_NOCTTY);
  if (slave == -1)
    {
#if 0
      if (buf != _buf)
        free (buf);
#endif
      goto fail;
    }

  /* XXX Should we ignore errors here?  */
  if(termp)
    tcsetattr (slave, TCSAFLUSH, termp);
  if (winp)
    ioctl (slave, TIOCSWINSZ, winp);

  *amaster = master;
  *aslave = slave;
  if (name != NULL)
    strcpy (name, buf);

#if 0
  if (buf != _buf)
    free (buf);
#endif
  return 0;

 fail:
  close (master);
  return -1;
}

void usage(char *progname) {
  fprintf(stderr,
	  "%s\n"
	  "Usage:\n"
	  " %s hostname [[--phone <phone number>] [--quirks ISP_NAME] -- ][ pppd options]\n"
	  "\nOr using pppd option pty: \n"
	  " pty \" %s hostname --nolaunchpppd [--phone <phone number>] [--quirks ISP_NAME]\"\n"
	  "Currently recognized ISP_NAMEs for quirks are BEZEQ_ISRAEL\n",
	  version, progname, progname);
  log("%s called with wrong arguments, program not started.", progname);
  
  exit(1);
}

static int signaled = 0;

void do_nothing(int sig) { 
    /* do nothing signal handler. Better than SIG_IGN. */
    signaled = 1;
}

sigjmp_buf env;
void sighandler(int sig) {
  siglongjmp(env, 1);
}

/* TODO: redesign to avoid longjmp/setjmp.  Several variables here
   have a volatile qualifier to silence warnings from gcc < 3.0.
   Remove the volatile qualifiers when longjmp/setjmp are removed. */

int main(int argc, char **argv, char **envp) {
  struct in_addr inetaddr;
  volatile int callmgr_sock = -1;
  char ttydev[PATH_MAX];
  int pty_fd, tty_fd, rc;
  volatile pid_t parent_pid, child_pid;
  u_int16_t call_id, peer_call_id;
  int pppdargc;
  char **pppdargv;
  char phonenrbuf[65]; /* maximum length of field plus one for the trailing
                        * '\0' */
  char * volatile phonenr = NULL;
  volatile int launchpppd = 1;
  if (argc < 2)
    usage(argv[0]);

  /* Step 1a: Get IP address for the hostname in argv[1] */
  inetaddr = get_ip_address(argv[1]);

  /* step 1b: Find the ppp options, extract phone number */
  argc--;
  argv++;
  while(1){ 
      /* structure with all recognised options for pptp */
      static struct option long_options[] = {
          {"phone", 1, 0, 0},  
          {"nolaunchpppd", 0, 0, 0},  
	  {"quirks", 1, 0, 0},
          {0, 0, 0, 0}
      };
      int option_index = 0;
      int c;
      opterr=0; /* suppress "unrecognised option" message, here
                 * we assume that it is a pppd option */
      c = getopt_long (argc, argv, "", long_options, &option_index);
      if( c==-1) break;  /* no more options */
      switch (c) {
        case 0: 
            if(option_index == 0) { /* --phone specified */
                /* copy it to a buffer, as the argv's will be overwritten by 
                 * inststr() */
                strncpy(phonenrbuf,optarg,sizeof(phonenrbuf));
                phonenrbuf[sizeof(phonenrbuf)-1]='\0';
                phonenr=phonenrbuf;
            }else if(option_index == 1) {/* --nolaunchpppd specified */
                  launchpppd=0;
            }else if(option_index == 2) {/* --quirks specified */
                if (set_quirk_index(find_quirk(optarg)))
                    usage(argv[0]);
            }
            /* other pptp options come here */
            break;
        case '?': /* unrecognised option, treat it as the first pppd option */
            /* fall through */
        default:
            c = -1;
            break;
      }
      if( c==-1) break;  /* no more options for pptp */
    }
  pppdargc = argc - optind;
  pppdargv = argv + optind;
 
  /* Step 3: Find an open pty/tty pair. */
  if(launchpppd){
      rc = my_openpty (&pty_fd, &tty_fd, ttydev, NULL, NULL);
      if (rc < 0) { 
          close(callmgr_sock); 
          fatal("Could not find free pty.");
      }
  
      /* Step 4: fork and wait. */
      signal(SIGUSR1, do_nothing); /* don't die */
      parent_pid = getpid();
      switch (child_pid = fork()) {
      case -1:
        fatal("Could not fork pppd process");

      case 0: /* I'm the child! */
        close (tty_fd);
        signal(SIGUSR1, SIG_DFL);
        child_pid = getpid();
        break;
      default: /* parent */
        close (pty_fd);
        /*
         * There is still a very small race condition here.  If a signal
         * occurs after signaled is checked but before pause is called,
         * things will hang.
         */
        if (!signaled) {
            pause(); /* wait for the signal */
        }
        launch_pppd(ttydev, pppdargc, pppdargv); /* launch pppd */
        perror("Error");
        fatal("Could not launch pppd");
      }
  } else { /* ! launchpppd */
      pty_fd=tty_fd=0;
      child_pid=getpid();
      parent_pid=0; /* don't kill pppd */
  }

  do {
    /*
     * Step 2: Open connection to call manager
     *         (Launch call manager if necessary.)
     */
    callmgr_sock = open_callmgr(inetaddr, phonenr, argc, argv, envp);

  /* Step 5: Exchange PIDs, get call ID */
  } while (get_call_id(callmgr_sock, parent_pid, child_pid, 
	               &call_id, &peer_call_id) < 0);

  /* Step 5b: Send signal to wake up pppd task */
  if(launchpppd){
      kill(parent_pid, SIGUSR1);
      sleep(2);
  }
 
  {
    char buf[128];
    snprintf(buf, sizeof(buf), "pptp: GRE-to-PPP gateway on %s", ttyname(tty_fd));
    inststr(argc,argv,envp, buf);
  }

  if (sigsetjmp(env, 1)!=0) goto shutdown;
  signal(SIGINT,  sighandler);
  signal(SIGTERM, sighandler);
  signal(SIGKILL, sighandler);
 
  /* Step 6: Do GRE copy until close. */
  pptp_gre_copy(call_id, peer_call_id, pty_fd, inetaddr);

shutdown:
  /* on close, kill all. */
  if(launchpppd)
      kill(parent_pid, SIGTERM);
  close(pty_fd);
  close(callmgr_sock);
  sleep(3);     /* give ctrl manager a chance to exit */
  exit(0);
}

struct in_addr get_ip_address(char *name) {
  struct in_addr retval;
  struct hostent *host = gethostbyname(name);
  if (host==NULL) {
    if (h_errno == HOST_NOT_FOUND)
      fatal("gethostbyname: HOST NOT FOUND");
    else if (h_errno == NO_ADDRESS)
      fatal("gethostbyname: NO IP ADDRESS");
    else
      fatal("gethostbyname: name server error");
  }
  
  if (host->h_addrtype != AF_INET)
    fatal("Host has non-internet address");
  
  memcpy(&retval.s_addr, host->h_addr, sizeof(retval.s_addr));
  return retval;
}

int open_callmgr(struct in_addr inetaddr, char *phonenr, int argc, char **argv, char **envp)
{
  /* Try to open unix domain socket to call manager. */
  struct sockaddr_un where;
  const int NUM_TRIES = 3;
  int i, fd;
  pid_t pid;
  int status;

  /* Open socket */
  if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
    fatal("Could not create unix domain socket: %s", strerror(errno));
  }

  /* Make address */
  where.sun_family = AF_UNIX;
  snprintf(where.sun_path, sizeof(where.sun_path), 
	   PPTP_SOCKET_PREFIX "%s", inet_ntoa(inetaddr));

  for (i=0; i<NUM_TRIES; i++) {
    if (connect(fd, (struct sockaddr *) &where, sizeof(where)) < 0) {
      /* couldn't connect.  We'll have to launch this guy. */

      unlink (where.sun_path);	

      /* fork and launch call manager process */
      switch (pid=fork()) {
      case -1: /* failure */
	  fatal("fork() to launch call manager failed.");
      case 0: /* child */
	  {
	      close (fd);
	      launch_callmgr(inetaddr, phonenr, argc,argv,envp);
	  }
      default: /* parent */
	  waitpid(pid, &status, 0);
	  if (status!=0)
	      fatal("Call manager exited with error %d", status);
	  break;
      }
      sleep(1);
    }
    else return fd;
  }
  close(fd);
  fatal("Could not launch call manager after %d tries.", i);
  return -1;   /* make gcc happy */
}

void launch_callmgr(struct in_addr inetaddr, char *phonenr, int argc,
        char**argv,char**envp) 
{
      int callmgr_main(int argc, char**argv, char**envp);
      char *my_argv[3] = { argv[0], inet_ntoa(inetaddr), phonenr };
      char buf[128];
      snprintf(buf, sizeof(buf), "pptp: call manager for %s", my_argv[1]);
      inststr(argc,argv,envp,buf);
      exit(callmgr_main(3, my_argv, envp));
      /*
      const char *callmgr = PPTP_CALLMGR_BINARY;
      execlp(callmgr, callmgr, inet_ntoa(inetaddr), NULL);
      fatal("execlp() of call manager [%s] failed: %s", 
	  callmgr, strerror(errno));
      */
}

/* XXX need better error checking XXX */
int get_call_id(int sock, pid_t gre, pid_t pppd, 
		 u_int16_t *call_id, u_int16_t *peer_call_id)
{
  u_int16_t m_call_id, m_peer_call_id;
  /* write pid's to socket */
  /* don't bother with network byte order, because pid's are meaningless
   * outside the local host.
   */
  int rc;
  rc = write(sock, &gre, sizeof(gre));
  if (rc != sizeof(gre))
      return -1;
  rc = write(sock, &pppd, sizeof(pppd));
  if (rc != sizeof(pppd))
      return -1;
  rc = read(sock,  &m_call_id, sizeof(m_call_id));
  if (rc != sizeof(m_call_id))
      return -1;
  rc = read(sock,  &m_peer_call_id, sizeof(m_peer_call_id));
  if (rc != sizeof(m_peer_call_id))
      return -1;
  /*
   * XXX FIX ME ... DO ERROR CHECKING & TIME-OUTS XXX
   * (Rhialto: I am assuming for now that timeouts are not relevant
   * here, because the read and write calls would return -1 (fail) when
   * the peer goes away during the process. We know it is (or was)
   * running because the connect() call succeeded.)
   */
  *call_id = m_call_id;
  *peer_call_id = m_peer_call_id;

  return 0;
}

void launch_pppd(char *ttydev, int argc, char **argv) {
  char *new_argv[argc+4]; /* XXX if not using GCC, hard code a limit here. */
  int i = 0, j;

  new_argv[i++] = PPPD_BINARY;
#ifdef USER_PPP
  new_argv[i++] = "-direct";
  /* ppp expects to have stdin connected to ttydev */
  if ((j = open(ttydev, O_RDWR)) == -1)
    fatal("Cannot open %s: %s", ttydev, strerror(errno));
  if (dup2(j, 0) == -1)
    fatal("dup2 failed: %s", strerror(errno));
  close(j);
#else
  new_argv[i++] = ttydev;
  new_argv[i++] = "38400";
#endif
  for (j=0; j<argc; j++)
    new_argv[i++] = argv[j];
  new_argv[i] = NULL;
  execvp(new_argv[0], new_argv);
}

/*************** COMPILE call manager into same binary *********/
#define main       callmgr_main
#define sighandler callmgr_sighandler
#define do_nothing callmgr_do_nothing
#define env        callmgr_env
#include "pptp_callmgr.c"
