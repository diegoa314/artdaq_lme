 // This file (periodic_cmd_stats.cc) was created by Ron Rechenmacher <ron@fnal.gov> on
 // Jan  5, 2017. "TERMS AND CONDITIONS" governing this file are in the README
 // or COPYING file. If you do not have such a file, one can be obtained by
 // contacting Ron or Fermi Lab in Batavia IL, 60510, phone: 630-840-3000.
 // $RCSfile: periodic_cmd_stats.cc,v $
static const char*rev="$Revision: 1.19 $$Date: 2018/06/28 21:14:28 $";

//    make periodic_cmd_stats
// OR make periodic_cmd_stats CXX="g++ -std=c++0x -DDO_TRACE"

#define USAGE "\
   usage: %s --cmd=<cmd>\n\
examples: %s --cmd='sleep 25'   # this will take about a minute\n\
          %s --cmd='taskset -c 4 dd if=/dev/zero of=/dev/sdf bs=50M count=500 oflag=direct' --disk=sdf\n\
          %s --cmd='dd if=/dev/zero of=t.dat count=500' --stat='md*MB/s?/proc/diskstats?/md[0-3]/?$10?(1.0/2048)?yes'\n\
          gnuplot -e png=0 `/bin/ls -t periodic_*_stats.out|head -1`\n\
For each cmd, record CPU info. Additionally, record total system CPU\n\
(can be >100 w/ multicore), file system Cached and Dirty %%, plus any\n\
other stats specified by --stats options\n\
options:\n\
--cmd=        can have multiple\n\
--Cmd=        run cmd, but dont graph CPU (can have multiple)\n\
--pid=        graph comma or space sep. list of pids (getting cmd from /proc/)\n\
--disk=       automatically build --stat for disk(s)\n\
--stat=       desc?file?linespec?fieldspec?multiplier?rate\n\
		      builtin = CPUnode,Cached,Dirty\n\
              cmd-builtin = CPUcmdN, CPU+cmdN\n\
--out-dir=    output dir (for 2+ output files; stats and cmd+)\n\
\n\
--period=     (float) default=%s\n\
--pre=        the number of periods wait before exec of --cmd\n\
--post=       the number of periods to loop after cmd exits\n\
--graph=      add to graph any possibly included non-graphed metrics\n\
\n\
--no-defaults for no default stats (ie. cpu-parent,cpu-children)\n\
--init=       default dropcache if root. NOT implemented yet\n\
--duration=\n\
--comment=\n\
--yrange=400:1400\n\
--y2max=\n\
--y2incr=\n\
--pause=0\n\
--sys-iowait,-w     include the system iowait on the graph\n\
--cmd-iowait      include cmd iowait on the graph\n\
--fault,-f\n\
", basename(argv[0]),basename(argv[0]),basename(argv[0]),basename(argv[0]),opt_period

enum { s_desc, s_file, s_linespec, s_fieldspec, s_multiplier, s_rate };

#include <stdio.h>				// printf
#include <getopt.h>	// getopt_long, {no,required,optional}_argument, extern char *optarg; extern int opt{ind,err,opt}
#include <unistd.h>				// getpid, sysconf
#include <sys/wait.h>			// wait
#include <fcntl.h>				// O_WRONLY|O_CREAT, open
#include <sys/time.h>			/* gettimeofday, timeval */
#include <signal.h>				/* sigaction, siginfo_t, sigset_t */
#include <sys/utsname.h>  		// uname
#include <string>
#include <sstream>				// std::stringstream
#include <vector>
#include <thread>
#include <fstream>				// std::ifstream

#ifdef DO_TRACE
# define TRACE_NAME "periodic_cmd_stats"
# include "trace.h"
#else
# include <stdarg.h>			/* va_list */
# include <string.h>			/* memcpy */
# include <string>
static void trace_ap(const char*msg,va_list ap)
{   char m_[1024]; unsigned len=strlen(msg);
	len=len<(sizeof(m_)-2)?len:(sizeof(m_)-2); memcpy(m_,msg,len+1);
	if(m_[len-1]!='\n') memcpy(&(m_[len]),"\n",2);
	vprintf(m_,ap);va_end(ap);
}
static void trace_p(const char*msg,...)__attribute__((format(printf,1,2)));
static void trace_p(const char*msg,...)
{   va_list ap;va_start(ap,msg); trace_ap(msg        ,ap); va_end(ap);
}
static void trace_p(const std::string msg,...)
{   va_list ap;va_start(ap,msg); trace_ap(msg.c_str(),ap); va_end(ap);
}
# define TRACE(lvl,...) do if(lvl<=1)trace_p(__VA_ARGS__);while(0)
# define TRACE_CNTL( xyzz,... )
#endif

/* GLOBALS */
int           opt_v=1;
char*         opt_init=NULL;
std::vector<std::string> opt_cmd;
std::vector<std::string> opt_Cmd;
std::string   opt_pid;
std::string   opt_disk;
std::string   opt_stats;
std::string   opt_outdir("");
std::string   opt_graph("CPUnode,Cached,Dirty,Free"); // CPU+ will always be graphed
const char*   opt_period="5.0";
std::string   opt_comment;
int           opt_pre=6;	// number of periods to sleepB4exec
int           opt_post=6;
int           opt_ymin=0;
int           opt_ymax=2000;
int           opt_yincr=200;
int           opt_y2max=200;
int           opt_y2incr=20;
int           opt_sys_iowait=0;
int           opt_cmd_iowait=0;
int           opt_fault=0;

std::vector<pid_t> g_pid_vec;

void charreplace( char* instr, char oldc, char newc )
{
	while (*instr) {
		if (*instr == oldc)
			*instr=newc;
		++instr;
	}
}


void parse_args( int argc, char	*argv[] )
{
	char *cp;
	// parse opt, optargs, and args
	while (1) {
		int opt;
		static struct option long_options[] = {
			// name            has_arg          *flag  val
			{ "help",          no_argument,      0,    'h' },
			{ "init",          required_argument,0,    'i' },
			{ "cmd",           required_argument,0,    'c' },
			{ "Cmd",           required_argument,0,    'C' },
			{ "disk",          required_argument,0,    'd' },
			{ "stat",          required_argument,0,    's' },
			{ "out-dir",       required_argument,0,    'o' },
			{ "period",        required_argument,0,    'p' },
            { "sys-iowait",    no_argument,      0,    'w' },
            { "fault",         no_argument,      0,    'f' },
			{ "pid",           required_argument,0,    'P' },
			{ "ymax",          required_argument,0,     1 },
			{ "yincr",         required_argument,0,     2 },
			{ "y2max",         required_argument,0,     3 },
			{ "y2incr",        required_argument,0,     4 },
			{ "pre",           required_argument,0,     5 },
			{ "post",          required_argument,0,     6 },
			{ "graph",         required_argument,0,     7 },
			{ "yrange",        required_argument,0,     8 },
			{ "comment",       required_argument,0,     9 },
			{ "cmd-iowait",    no_argument,      0,    10 },
			{     0,              0,             0,     0 }
		};
		opt = getopt_long( argc, argv, "?hvqVi:c:C:d:s:o:p:P:wf",
						  long_options, NULL );
		if (opt == -1) break;
		switch (opt) {
		case '?': case 'h': printf(USAGE);   exit(0);                    break;
		case 'V': printf("%s\n",rev);       exit(0);                     break;
		case 'v': ++opt_v;                                               break;
		case 'q': --opt_v;                                               break;
		case 'i': opt_init=optarg;                                       break;
		case 'c': opt_cmd.push_back(optarg);                             break;
		case 'C': opt_Cmd.push_back(optarg);                             break;
		case 'd': if(opt_disk.size())opt_disk=opt_disk+","+optarg;else opt_disk=optarg;break;
		case 's': if(opt_stats.size())opt_stats=opt_stats+","+optarg;else opt_stats=optarg;break;
		case 'o': opt_outdir=std::string(optarg)+"/";                    break;
		case 'p': opt_period=optarg;                                     break;
		case 'w': opt_sys_iowait=1;                                            break;
		case 'f': opt_fault=1;                                           break;
		case 'P': charreplace(optarg,' ',',');
			      if(opt_pid.size())opt_pid=opt_pid+","+optarg;
			      else opt_pid=optarg;
			      break;
		case 1:   opt_ymax=strtoul(optarg,NULL,0);                       break;
		case 2:   opt_yincr=strtoul(optarg,NULL,0);                      break;
		case 3:   opt_y2max=strtoul(optarg,NULL,0);                      break;
		case 4:   opt_y2incr=strtoul(optarg,NULL,0);                     break;
		case 5:   opt_pre=strtoul(optarg,NULL,0);                        break;
		case 6:   opt_post=strtoul(optarg,NULL,0);                       break;
		case 7:   opt_graph+=std::string(",")+optarg;                    break;
		case 8:   opt_ymin =strtoul(optarg,NULL,0);
			      cp = strstr(optarg,":")+1;
			      opt_ymax =strtoul(cp,NULL,0);
				  if ((cp=strstr(cp,":"))) {
					  ++cp;
					  opt_yincr=strtoul(strstr(cp,":")+1,NULL,0);
				  } else
					  opt_yincr=(opt_ymax-opt_ymin)/5;
				  break;
		case 9:   opt_comment=optarg;                                    break;
		case 10:  opt_cmd_iowait=1;                                      break;
		default:
			printf( "?? getopt returned character code 0%o ??\n", opt );
			exit( 1 );
		}
	}
}	/* parse_args */

void perror_exit( const char *msg, ... )
{	char buf[1024];
	va_list ap;va_start(ap,msg);
	vsnprintf( buf, sizeof(buf), msg, ap );
	va_end(ap);
	TRACE( 0, "%s", buf );
	perror( buf ); exit(1);
}

//void atfork_trace(void) { TRACE( 3, "process %d forking", getpid() ); }
/* iofd is in/out
   if iofd[x]==-1 then create a pipe for that index, x, and return the appropriate pipe fd in iofd[x]
   else if iofd[x]!=x, dup2(iofd[x],x)
   else inherit
   Could add ==-2, then close???
 */
pid_t fork_execv( int close_start, int close_cnt, int sleepB4exec_us, int iofd[3], const char *cmd, char *const argv[], char *const env[] )
{
	int pipes[3][2];
	int lcl_iofd[3];
	for (auto ii=0; ii<3; ++ii) {
		lcl_iofd[ii]=iofd[ii];
		if (iofd[ii]==-1) {
			pipe(pipes[ii]);  /* pipes[ii][0] refers to the read end */
			iofd[ii]=ii==0?pipes[ii][1]:pipes[ii][0];
		}
	}
	pid_t pid=fork();
	if (pid < 0) perror_exit("fork");
	else if (pid == 0) { /* child */
		if (lcl_iofd[0]==-1) {  // deal with child stdin
			close(pipes[0][1]); // child closes write end of pipe which will be it's stdin
			int fd=dup2(pipes[0][0],0);
			TRACE( 3, "fork_execv dupped(%d) onto %d (should be 0)", pipes[0][0], fd );
			close(pipes[0][0]);
		}
		if (sleepB4exec_us) {
			// Do sleep before dealing with stdout/err incase we want TRACE to go to console
			//int sts=pthread_atfork( atfork_trace, NULL, NULL );
			usleep(sleepB4exec_us);
			TRACE( 1, "fork_execv sleep complete. sleepB4exec_us=%d sts=%d", sleepB4exec_us, 0/*sts*/ );
		}
		for (auto ii=1; ii<3; ++ii) { // deal with child stdout/err
			if (lcl_iofd[ii]==-1) {
				close(pipes[ii][0]);
				int fd=dup2(pipes[ii][1],ii);
				TRACE( 3, "fork_execv dupped(%d) onto %d (should be %d)", pipes[ii][1], fd,ii );
				close(pipes[ii][1]);
			} else if (lcl_iofd[ii]!=ii) {
				int fd=dup2(lcl_iofd[ii],ii);
				TRACE( 3, "fork_execv dupped(%d) onto %d (should be %d)", pipes[ii][1], fd,ii );
			}
		}
		for (auto ii=close_start; ii<(close_start+close_cnt); ++ii)
			close(ii);
		if (env)
			execve( cmd, argv, env );
		else
			execv( cmd, argv );
		exit(1);
	} else { // parent
		for (auto ii=0; ii<3; ++ii)
			if (lcl_iofd[ii]==-1)
				close(ii==0?pipes[ii][0]:pipes[ii][1]);
	}

	TRACE( 3, "fork_execv pid=%d", pid );
	return pid;
}   // fork_execv

uint64_t swapPtr( void *X )
{
	uint64_t x=(uint64_t)X;
	x = (x & 0x00000000ffffffff) << 32 |  (x & 0xffffffff00000000) >> 32;
	x = (x & 0x0000ffff0000ffff) << 16 |  (x & 0xfff0000fffff0000) >> 16;
	x = (x & 0x00ff00ff00ff00ff) <<  8 |  (x & 0xff00ff00ff00ff00) >>  8;
	return x;
}

/*
 *  Input to AWK can either be a file spec or a string.
 *  If input is string, the fork_execv call is told to create pipe for input.
 *  
 *  The run time duration of the AWK prooces can be determined via TRACE:
/home/ron/src
mu2edaq01 :^) tshow|egrep 'AWK b4 |AWK after read' |tdelta -d 1 -post /b4/ -stats | tail
1013 1489724640538688      2047 1116418481 13521   0   6  3 . AWK b4 fork_execv input=(nil)
1018 1489724640536624      1969 1111669678 13521   0   6  3 . AWK b4 fork_execv input=(nil)
1023 1489724640534717      1866 1107283893 13521   0   6  3 . AWK b4 fork_execv input=(nil)
1032 1489724640531756      2289 1100474359 13521   0  13  3 . AWK b4 fork_execv input=(nil)
cpu="0"
                  min      1821
                  max     49210
                  tot    293610
                  ave 2645.1351
                  cnt       111
--2017-03-17_08:13:23--
 */
static int g_devnullfd=-1;

// Run the awk script specified in awk_cmd on the file
std::string AWK( std::string const &awk_cmd, const char *file, const char *input )
{
	char readbuf[1024];
	ssize_t bytes=0, tot_bytes=0;
	char *const argv_[4]={ (char*)"/bin/gawk",
						   (char*)awk_cmd.c_str(),
						   (char*)file,
						   NULL };
	pid_t pid;;
	int infd=0;
	if (g_devnullfd == -1)
		g_devnullfd=open("/dev/null",O_WRONLY);
	if (input != NULL) {
		infd=-1;
	}
	//int iofd[3]={infd,-1,g_devnullfd};
	int iofd[3]={infd,-1,2};// make stdin=infd, create pipr for stdout, inherit stderr
	TRACE( 3, "AWK b4 fork_execv input=%p", (void*)input );
	char *env[1];
	env[0]=NULL;				// mainly do not want big LD_LIBRARY_PATH
	pid=fork_execv(0,0/*closeCnt*/,0,iofd,"/bin/gawk",argv_,env);
	if(input/*||iofd[0]!=0*/) {
		int xx=strlen(input);
		int sts=write(iofd[0],input,xx);
		if (sts != xx)
			perror("write AWK stdin");
		close(iofd[0]);
		while ((bytes=read(iofd[1],&readbuf[tot_bytes],sizeof(readbuf)-tot_bytes)) != 0) {
			TRACE( 3, "AWK while bytes=read > 0 bytes=%zd readbuf=0x%016lx errno=%d", bytes, swapPtr(&readbuf[tot_bytes]), errno );
			if (bytes == -1) {
				if (errno == EINTR) continue;
				break;
			}
			tot_bytes+=bytes;
		}
		TRACE( 3, "AWK after read tot="+std::to_string((long long unsigned)tot_bytes)+" bytes="+std::to_string((long long unsigned)bytes)+" input="+std::string(input) );
	} else {
		while ((bytes=read(iofd[1],&readbuf[tot_bytes],sizeof(readbuf)-tot_bytes)) > 0)
			tot_bytes+=bytes;
		TRACE( 3, "AWK after read tot=%zd bytes=%zd [0]=0x%x input=%p", tot_bytes, bytes, readbuf[0], (void*)input );
	}
	readbuf[tot_bytes>=0?tot_bytes:0]='\0';
	close(iofd[1]);
	TRACE( 3, "AWK after close child stdout. child pid=%d", pid );
#if 0
	int status;
	pid_t done_pid = waitpid(pid,&status,0);
	TRACE( 3, "AWK after wait pid=%d done_pid=%d status=%d(0x%x)"
	      , pid, done_pid, status, status );
#endif
	return std::string(readbuf);
} // AWK


// separate string and _add_to_ vector
void string_addto_vector( std::string &instr, std::vector<std::string> &outvec, char delim )
{
	std::stringstream ss(instr);
	while( ss.good() )
	{	std::string substr;
		std::getline( ss, substr, delim );
		outvec.push_back( substr );
	}
}

uint64_t gettimeofday_us( void )   //struct timespec *ts )
{   struct timeval tv;
    gettimeofday( &tv, NULL );
	// if (ts) {
	// 	ts->tv_sec  = tv.tv_sec;
	// 	ts->tv_nsec = tv.tv_usec * 1000;
	// }
    return (uint64_t)tv.tv_sec*1000000+tv.tv_usec;
}   /* gettimeofday_us */

#define DATA_START " DATA START"
#define GNUPLOT_PREFIX (const char *)"\
#!/usr/bin/env gnuplot\n\
#         ./$0\n\
# OR\n\
#         gnuplot -e 'ymin=400;ymax=1400' ./$0\n\
# OR try\n\
#         gnuplot -e 'duration_s=35;set multiplot' ./gnuplot.gnuplot ./gnuplot.1.gnuplot -e 'set nomultiplot;pause -1'\n\
if(!exists('ARG0')) ARG0=''  # for version 4, use: gnuplot -e ARG0=hello\n\
print 'ARG0=',ARG0   # ARG0.. automatically define in gnuplot version 5+\n\
if(!exists('ymin')) ymin=%d\n\
if(!exists('ymax'))  ymax=%d\n\
if(!exists('yincr'))  yincr=%d\n\
if(!exists('y2max'))   y2max=%d\n\
if(!exists('y2incr'))   y2incr=%d\n\
if(!exists('png'))       png=1\n\
if(!exists('duration_s')) duration_s=0\n\
if(!exists('width')) width=512\n\
if(!exists('height')) height=384\n\
thisPid=system('echo `ps -p$$ -oppid=`')\n\
thisFile=system('ls -l /proc/'.thisPid.\"/fd | grep -v pipe: | tail -1  | sed -e 's/.*-> //'\")\n\
\n\
set title \"Disk Write Rate and %%CPU vs. time\\n%s %s %s%s\" # cmd and/or comment at end\n\
set xdata time\n\
tfmt='%%Y-%%m-%%dT%%H:%%M:%%S'     # try to use consistent format\n\
set timefmt '%%Y-%%m-%%dT%%H:%%M:%%S'\n\
set xlabel 'time'\n\
set grid xtics back\n\
xstart=system(\"awk '/^....-..-..T/{print$1;exit}' \".thisFile)\n\
xend=system(\"awk 'END{print$1}' \".thisFile)\n\
print 'xstart='.xstart.' xend='.xend.' duration=',strptime(tfmt,xend)-strptime(tfmt,xstart)\n\
if(duration_s>0) end_t=strptime(tfmt,xstart)+duration_s; else end_t=strptime(tfmt,xend)\n\
set xrange [xstart:end_t]\n\
\n\
set ylabel '%s'\n\
set ytics nomirror\n\
if(ymax==0) set yrange [ymin:*];\\\n\
else        set yrange [ymin:ymax];set ytics yincr\n\
set grid ytics back\n\
\n\
set y2label '%%CPU, %%MemTotal'\n\
set y2tics autofreq\n\
if(y2max==0) set y2range [0:*];\\\n\
else         set y2range [0:y2max];set y2tics y2incr\n\
set pointsize .6\n\
\n\
if(png==1) set terminal png size width,height;\\\n\
           pngfile=system( 'echo `basename '.thisFile.' .out`.png' );\\\n\
           set output pngfile;\\\n\
else       set terminal x11 size width,height\n\
\n\
plot \"< awk '/^#" DATA_START "/,/NEVER HAPPENS/' \".thisFile "


void sigchld_sigaction( int signo, siginfo_t *info, void *context __attribute__((__unused__)) )
{
	/* see man sigaction for description of siginfo_t */
	for (size_t ii=0; ii < g_pid_vec.size(); ++ii) {
		pid_t pid=g_pid_vec[ii];
		if (pid == info->si_pid) {
			TRACE( 2, "sigchld_sigaction signo=%d status=%d(0x%x) code=%d(0x%x) sending_pid=%d"
			      , signo
			      , info->si_status, info->si_status
			      , info->si_code,   info->si_code
			      , info->si_pid
			      );
			return;
		}
	}
	TRACE( 3, "sigchld_sigaction signo=%d status=%d(0x%x) code=%d(0x%x) sending_pid=%d"
	      , signo
	      , info->si_status, info->si_status
	      , info->si_code,   info->si_code
	      , info->si_pid
	      );
}


void read_proc_file( const char *file, char *buffer, int buffer_size )
{
	TRACE( 4, "read_proc_file b4 open proc file"+std::string(file) );
	int fd=open(file,O_RDONLY);
	int offset=0, sts=0;
	while (1) {
		sts=read(fd,&buffer[offset],buffer_size-offset);
		if (sts<=0) {
			sts=0;
			break;
		}
		offset+=sts;
	}
	buffer[sts+offset]='\0';
	close(fd);
	TRACE( 4, "read_proc_file after close "+std::string(file)+" read=%d offset=%d",sts,offset );
}


pid_t check_pid_vec( void )
{
	for (size_t ii=0; ii < g_pid_vec.size(); ) {
		pid_t pid=g_pid_vec[ii];
		int status;
		pid_t pp = waitpid( pid, &status, WNOHANG );
		TRACE( 3, "check_pid_vec %d=waitpid(pid=%d) errno=%d", pp, pid, errno );
		if (pp > 0)
			g_pid_vec.erase( g_pid_vec.begin()+ii );
		else if (pp == -1) {
			if (errno == ECHILD && kill(pid,0)==0)
				// there is a process, but not my child process
				++ii;
			else
				// some other error
				g_pid_vec.erase( g_pid_vec.begin()+ii );
		}
		else
			++ii;
	}
	if (g_pid_vec.size() == 0)
		return -1;
	else
		return 0;
}

void cleanup( void )
{
	TRACE( 1, "atexit cleanup g_pid_vec.size()=%zd\n", g_pid_vec.size() );
	for (std::vector<pid_t>::iterator pid=g_pid_vec.begin(); pid!=g_pid_vec.end(); ++pid) {
		kill( *pid, SIGHUP );
	}
}
#if (defined(__cplusplus)&&(__cplusplus>=201103L)) || (defined(__STDC_VERSION__)&&(__STDC_VERSION__>=201112L))
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wunused-parameter"   /* b/c of TRACE_XTRA_UNUSED */
#endif
void sigint_sigaction( int signo, siginfo_t *info, void *context )
{
	cleanup();
	exit( 1 );
}
#if (defined(__cplusplus)&&(__cplusplus>=201103L)) || (defined(__STDC_VERSION__)&&(__STDC_VERSION__>=201112L))
# pragma GCC diagnostic pop
#endif


int
main(  int	argc
     , char	*argv[] )
{
	struct timeval tv;
	int    post_periods_completed=0;
	parse_args( argc, argv );
	if (   (argc-optind)!=0
	    || (   opt_cmd.size()==0
		    && opt_pid.size()==0)) { //(argc-optind) is the number of non-opt args supplied.
		int ii;
		printf( "unexpected argument(s) %d!=0\n", argc-optind );
		for (ii=0; (optind+ii)<argc; ++ii)
			printf("arg%d=%s\n",ii+1,argv[optind+ii]);
		printf( USAGE ); exit( 0 );
	}

	std::vector<std::string> graphs;
	string_addto_vector( opt_graph, graphs, ',' );

	char motherboard[1024]={0};
    if (getuid() == 0) {
		FILE *fp=popen( "dmidecode | grep -m2 'Product Name:' | tail -1", "r" );
		fread( motherboard, 1, sizeof(motherboard), fp );
		pclose( fp );
	}
	TRACE( 1, "main - motherboard="+std::string(motherboard) );

	/* Note, when doing "waitpid" the wait would sometimes take a "long"
	   time (10's to 100's milliseconds; rcu???) If signal is generated
	   (i.e SA_NOCLDWAIT w/ sigchld_sigaction (not SIG_IGN)), it would
	   sometimes effect the read or write calls for the following AWK forks.
	   So, use SIG_IGN+SA_NOCLDWAIT.
	 */
	struct sigaction sigaction_s;
#ifndef DO_SIGCHLD
# define DO_SIGCHLD 1
#endif
#if DO_SIGCHLD
    sigaction_s.sa_sigaction = sigchld_sigaction;
	sigaction_s.sa_flags = SA_SIGINFO|SA_NOCLDWAIT;
#else
    sigaction_s.sa_handler = SIG_IGN;
	sigaction_s.sa_flags = SA_NOCLDWAIT;
#endif
	sigemptyset(&sigaction_s.sa_mask);
    sigaction( SIGCHLD, &sigaction_s, NULL );

    sigaction_s.sa_sigaction = sigint_sigaction;
	sigaction_s.sa_flags = SA_SIGINFO;
    sigaction( SIGINT, &sigaction_s, NULL );

	//may return 0 when not able to detect
	//long long unsigned concurentThreadsSupported = std::thread::hardware_concurrency();
	long long unsigned concurentThreadsSupported = sysconf(_SC_NPROCESSORS_ONLN);
	//TRACE_CNTL( "reset" ); TRACE_CNTL( "modeM", 1L );
	TRACE( 0, "main concurentThreadsSupported=%u opt_stats="+opt_stats, concurentThreadsSupported );

	char run_time[80];
	gettimeofday( &tv, NULL );
	strftime( run_time, sizeof(run_time), "%FT%H%M%S", localtime(&tv.tv_sec) );
	TRACE( 0, "main run_time="+std::string(run_time) );

	// get hostname
	struct utsname ubuf;
	uname( &ubuf );
	char *dot;
	if ((dot=strchr(ubuf.nodename,'.')) != NULL)
		*dot = '\0';
	std::string hostname(ubuf.nodename);
	TRACE( 1,"release="+std::string(ubuf.release)+" version="+std::string(ubuf.version) );

	// get system mem (KB)
	std::string memKB=AWK( "NR==1{print$2;exit}","/proc/meminfo",NULL );
	memKB = memKB.substr(0,memKB.size()-1); // remove trailing newline

	std::string dat_file_out(opt_outdir+"periodic_"+run_time+"_"+hostname+"_stats.out");

	double period=atof(opt_period);

	atexit( cleanup );
	pid_t pp;
	std::vector<std::string> pidfile;

	std::vector<std::string> stats;

	// For each cmd: create out file, fork process (with delay param),
	// add to stats vec to get CPU info, add to graphs vec to plot cmd CPU
	for (size_t ii=0; ii<opt_cmd.size(); ++ii) {
		char cmd_file_out[1024];
		snprintf(  cmd_file_out, sizeof(cmd_file_out), "%speriodic_%s_%s_cmd%zd.out"
		         , opt_outdir.c_str(), run_time, hostname.c_str(), ii  );
		int fd=open( cmd_file_out, O_WRONLY|O_CREAT,0666 );
		TRACE( 0, "main fd=%d opt_cmd="+opt_cmd[ii]+" cmd_file_out="+std::string(cmd_file_out), fd );
		int iofd[3]={0,fd,fd};	// redirect stdout/err to the cmd-out-file
		char *const argv_[4]={ (char*)"/bin/sh",
							   (char*)"-c",
							   (char*)opt_cmd[ii].c_str(),
							   NULL };
		g_pid_vec.push_back( fork_execv(0,0,(int)(period*opt_pre*1e6),iofd,"/bin/sh",argv_,NULL) );
		close(fd); // the output file has been given to the subprocess
		std::string pidstr=std::to_string((long long int)g_pid_vec[ii]);
		pidfile.push_back( "/proc/"+pidstr+"/stat" );
		//pidfile.push_back( "/proc/"+pidstr+"/task/"+pidstr+"/stat" );
		char desc[128], ss[1024];
		// field 14-17: Documentation/filesystems/proc.txt Table 1-4: utime stime cutime cstime
		snprintf(  ss, sizeof(ss), "CPUcmd%zd?%s?NR==1?$14+$15?1?yes", ii, pidfile[ii].c_str() );
		stats.push_back( ss );

		snprintf( desc, sizeof(desc), "CPU+cmd%zd", ii );
		graphs.push_back( desc ); // cmd0 is in the GNUPLOT_PREFIX
		snprintf(  ss, sizeof(ss), "%s?%s?NR==1?$14+$15+16+$17?1?yes", desc, pidfile[ii].c_str() );
		stats.push_back( ss );

		snprintf( desc, sizeof(desc), "WaitBlkIOcmd%zd", ii );
		if (opt_cmd_iowait) graphs.push_back( desc );
		snprintf(  ss, sizeof(ss), "%s?%s?NR==1?$42?1?yes", desc, pidfile[ii].c_str() );
		stats.push_back( ss );

		snprintf( desc, sizeof(desc), "Faultcmd%zd", ii );
		if (opt_fault) graphs.push_back( desc );
		snprintf(  ss, sizeof(ss), "%s?%s?NR==1?$10+$11+$12+$13?4096.0/1048576?yes", desc, pidfile[ii].c_str() );
		stats.push_back( ss );
	}
	for (size_t ii=0; ii<opt_Cmd.size(); ++ii) {
		char cmd_file_out[1024];
		snprintf(  cmd_file_out, sizeof(cmd_file_out), "%speriodic_%s_%s_cmd%zd.out"
		         , opt_outdir.c_str(), run_time, hostname.c_str(), ii+opt_cmd.size() );
		int fd=open( cmd_file_out, O_WRONLY|O_CREAT,0666 );
		TRACE( 0, "main fd=%d opt_Cmd="+opt_Cmd[ii]+" cmd_file_out="+std::string(cmd_file_out), fd );
		int iofd[3]={0,fd,fd};	// redirect stdout/err to the cmd-out-file
		char *const argv_[4]={ (char*)"/bin/sh",
							   (char*)"-c",
							   (char*)opt_Cmd[ii].c_str(),
							   NULL };
		g_pid_vec.push_back( fork_execv(0,0,(int)(period*opt_pre*1e6),iofd,"/bin/sh",argv_,NULL) );
		close(fd); // the output file has been given to the subprocess
		std::string pidstr=std::to_string((long long int)g_pid_vec[ii]);
		pidfile.push_back( "/proc/"+pidstr+"/stat" );
		//pidfile.push_back( "/proc/"+pidstr+"/task/"+pidstr+"/stat" );
		char desc[128], ss[1024];
		snprintf( desc, sizeof(desc), "CPU+cmd%zd", ii+opt_cmd.size() );
		snprintf(  ss, sizeof(ss), "CPUcmd%zd?%s?NR==1?$14+$15?1?yes", ii+opt_cmd.size(), pidfile[ii].c_str() );
		stats.push_back( ss );
		snprintf(  ss, sizeof(ss), "CPU+cmd%zd?%s?NR==1?$14+$15+16+$17?1?yes", ii+opt_cmd.size(), pidfile[ii].c_str() );
		stats.push_back( ss );
		// JUST DONT ADD THESE TO graphs
	}
	std::vector<std::string> pids;
	if (opt_pid.size())
		string_addto_vector( opt_pid, pids, ',' );
	for (size_t ii=0; ii<pids.size(); ++ii) {
		g_pid_vec.push_back( std::stoi(pids[ii]) );
		TRACE( 1, "pid=%s g_pid_vec.size()=%ld", pids[ii].c_str(), g_pid_vec.size() );
		pidfile.push_back( "/proc/"+pids[ii]+"/stat" );
		char desc[128], ss[1024];
		// field 14-17: Documentation/filesystems/proc.txt Table 1-4: utime stime cutime cstime
		snprintf(  ss, sizeof(ss), "CPUpid%zd?%s?NR==1?$14+$15?1?yes", ii, pidfile[ii].c_str() );
		stats.push_back( ss );

		std::ifstream t("/proc/"+pids[ii]+"/comm");
		std::string comm((std::istreambuf_iterator<char>(t)),
		                 std::istreambuf_iterator<char>());
		comm = comm.substr(0,comm.size()-1);  // strip nl

		snprintf( desc, sizeof(desc), "CPU+pid%zd_%s", ii, comm.c_str() );
		graphs.push_back( desc ); // cmd0 is in the GNUPLOT_PREFIX
		snprintf(  ss, sizeof(ss), "%s?%s?NR==1?$14+$15+16+$17?1?yes", desc, pidfile[ii].c_str() );
		stats.push_back( ss );

		snprintf( desc, sizeof(desc), "WaitBlkIOpid%zd", ii );
		if (opt_cmd_iowait) graphs.push_back( desc );
		snprintf(  ss, sizeof(ss), "%s?%s?NR==1?$42?1?yes", desc, pidfile[ii].c_str() );
		stats.push_back( ss );

		snprintf( desc, sizeof(desc), "Faultpid%zd", ii );
		if (opt_fault) graphs.push_back( desc );
		snprintf(  ss, sizeof(ss), "%s?%s?NR==1?$10+$11+$12+$13?4096.0/1048576?yes", desc, pidfile[ii].c_str() );
		stats.push_back( ss );
	}
	

	stats.push_back("CPUnode");
	stats.push_back("IOWait");  if (opt_sys_iowait) { graphs.push_back("IOWait"); }
	stats.push_back("Cached");
	stats.push_back("Dirty");
	stats.push_back("Free");

	if (opt_disk.size()) {
		std::vector<std::string> tmp;
		string_addto_vector( opt_disk, tmp, ',' );
		for (std::vector<std::string>::iterator dk=tmp.begin(); dk!=tmp.end(); ++dk) {
			// /proc/diskstat has 11 field after an initial 3 (14 total) for each device
			// The 7th field after the device name (the 10th field total) is # of sectors written.
			// Sectors appear to be 512 bytes. So, deviding by 2048 converts to MBs.
			std::string statstr=*dk+"_wrMB/s?/proc/diskstats?/"+*dk+"/?$10?(1.0/2048)?yes";
			stats.push_back(statstr);
			std::vector<std::string> stat_spec;
			string_addto_vector( statstr, stat_spec, '?' );
			graphs.push_back( stat_spec[s_desc] );

			statstr=*dk+"_rdMB/s?/proc/diskstats?/"+*dk+"/?$6?(1.0/2048)?yes";
			stats.push_back(statstr);
			stat_spec.clear();
			string_addto_vector( statstr, stat_spec, '?' );
			//graphs.push_back( stat_spec[s_desc] ); // don't add read by default -- can be added with --graph
		}
	}

	if (opt_stats.size()) {
		std::vector<std::string> tmp_stats;
		string_addto_vector( opt_stats, tmp_stats, ',' );
		for (std::vector<std::string>::iterator st=tmp_stats.begin();
			 st!=tmp_stats.end(); ++st) {
			stats.push_back(*st);
			std::vector<std::string> stat_spec;
			string_addto_vector( *st, stat_spec, '?' );
			graphs.push_back( stat_spec[s_desc] );
		}
	}

	std::vector<long>        pre_vals;
	std::vector<double>      multipliers;
	std::vector<std::vector<std::string>> spec2(stats.size());
	std::vector<std::string> awkCmd;

	std::string header_str( "#" DATA_START "\n#_______time_______" );

	int outfd=open(dat_file_out.c_str(),O_WRONLY|O_CREAT,0777);
	//FILE *outfp=stdout;
	FILE *outfp = fdopen(outfd,"w");

	std::string cmd_comment("");
	if (opt_cmd.size())
		cmd_comment += "\\ncmd: "+opt_cmd[0];
	if (opt_comment.size())
		cmd_comment += "\\ncomment: " + opt_comment;
	fprintf( outfp, GNUPLOT_PREFIX, opt_ymin, opt_ymax,  opt_yincr,  opt_y2max,  opt_y2incr
	        , run_time, hostname.c_str(), ubuf.release
	        , cmd_comment.c_str()
	        , "disk write MB/s" );

	uint64_t t_start=gettimeofday_us();

	// build header string and get initial values for "rate" stats
	bool first_graph_spec_added=false;
	for (size_t ii=0; ii<stats.size(); ++ii) {
		std::vector<std::string> stat_spec;
		string_addto_vector( stats[ii], stat_spec, '?' );
		if      (stat_spec[s_desc]=="CPUnode" && stat_spec.size()==1)
			// Ref. Documentation/filesystems/proc.txt: user+nice+system (skip idle) +iowait+irq+softirq+steal (skip guest)
			stats[ii]+="?/proc/stat?/^cpu[^0-9]/?$2+$3+$4+$6+$7+$8+$9?1.0/"+std::to_string(concurentThreadsSupported)+"?yes";
		else if (stat_spec[s_desc]=="IOWait"  && stat_spec.size()==1)
			stats[ii]+="?/proc/stat?/^cpu[^0-9]/?$6?1.0/"+std::to_string(concurentThreadsSupported)+"?yes";
		else if (stat_spec[s_desc]=="Cached"  && stat_spec.size()==1)
			stats[ii]+="?/proc/meminfo?/^(Cached|Buffers):/?$2?1?no";
		else if (stat_spec[s_desc]=="Dirty"  && stat_spec.size()==1)
			stats[ii]+="?/proc/meminfo?/^Dirty:/?$2?1?no";
		else if (stat_spec[s_desc]=="Free"  && stat_spec.size()==1)
			stats[ii]+="?/proc/meminfo?/^MemFree:/?$2?1?no";

		header_str += " "+stat_spec[s_desc];

		string_addto_vector( stats[ii], spec2[ii], '?' );
		char awk_cmd[1024];
		snprintf(  awk_cmd, sizeof(awk_cmd), "%s{vv+=%s}END{print vv}"
		         //snprintf(  awk_cmd, sizeof(awk_cmd), "%s{vv+=%s;print \"vv now\",vv > \"/dev/stderr\";}END{print vv}"
		         , spec2[ii][s_linespec].c_str(), spec2[ii][s_fieldspec].c_str() );
		awkCmd.push_back(awk_cmd);

		std::string stat=AWK( awkCmd.back(), spec2[ii][s_file].c_str(), NULL );

		pre_vals.push_back(atol(stat.c_str()));
		multipliers.push_back(atof(AWK( "BEGIN{print "+spec2[ii][s_multiplier]+"}","/dev/null",NULL ).c_str()) );
		//fprintf( stderr, " l=%s", spec2[ii][s_linespec].c_str() );
		for (size_t jj=0; jj<graphs.size(); ++jj)
			if (graphs[jj] == stat_spec[s_desc]) {
				if (first_graph_spec_added) fprintf( outfp, ",\\\n  '' " );
				if      (strncmp(stat_spec[s_desc].c_str(),"CPU",3)==0)
					fprintf( outfp, "using 1:%zd title '%s' w linespoints axes x1y2", ii+2, stat_spec[s_desc].c_str() );
				else if (stat_spec[s_desc] == "Cached" || stat_spec[s_desc] == "Dirty" || stat_spec[s_desc] == "Free")
					fprintf( outfp, "using 1:($%zd/%s*100) title '%s%%' w linespoints axes x1y2", ii+2, memKB.c_str(), stat_spec[s_desc].c_str() );
				else if (stat_spec[s_desc].substr(0,6) == "CPUcmd" || stat_spec[s_desc].substr(0,6) == "CPU+cm")
					fprintf( outfp, "using 1:%zd title '%s' w linespoints axes x1y2", ii+2, stat_spec[s_desc].c_str() );
				else if (stat_spec[s_desc].substr(0,12) == "WaitBlkIOcmd" )
					fprintf( outfp, "using 1:%zd title '%s' w linespoints axes x1y2", ii+2, stat_spec[s_desc].c_str() );
				else
					fprintf( outfp, "using 1:%zd title '%s' w linespoints axes x1y1", ii+2, stat_spec[s_desc].c_str() );
				first_graph_spec_added=true;
			}
	}
	header_str += " #\n";

	fprintf( outfp, "\nif(png==0) pause -1 'Press Enter/Return or ^C to finish'\n\
exit\n" );

	// print the cmds
	fprintf( outfp, "cmds:\n" );
	for (size_t ii=0; ii<opt_cmd.size(); ++ii) {
		std::string ss=opt_cmd[ii]+"\n";
		fprintf( outfp, "%s", ss.c_str() );
	}

	// print the specs
	fprintf( outfp, "stats:\n" );
	for (size_t ii=0; ii<stats.size(); ++ii) {
		std::string ss=stats[ii]+"\n";
		fprintf( outfp, "%s", ss.c_str() );
	}

	// now print header
	fprintf( outfp, "%s", header_str.c_str() ); fflush( outfp );

	std::string tmpdbg("main lp=%d done stat%zd=%ld rate=%f ");
	//char tmpdbgbuf[128];
	char proc_stats[8192];
	char *awk_in;
	int lp;

	// -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
	// wait a period and then start collecting the stats
 eintr1:
	int64_t t_sleep=(t_start+(uint64_t)(period*1e6))-gettimeofday_us();
	if (t_sleep > 0) {
		int sts=usleep( t_sleep );
		TRACE( 3,"main usleep sts=%d errno=%d",sts, errno );
		if(errno == EINTR)
			goto eintr1;
	}

#   define MAX_LP 600
	for (lp=2; lp<MAX_LP; ++lp) {
		char str[80];
		gettimeofday( &tv, NULL );
		strftime( str, sizeof(str), "%FT%T", localtime(&tv.tv_sec) );
		//fprintf(outfp, "%s.%ld", str, tv.tv_usec/100000 );
		fprintf(outfp, "%s", str );
		std::string prv_file("");
		for (size_t ii=0; ii<stats.size(); ++ii) {
			TRACE( 3, "main lp=%d start stat%zd", lp, ii );
			char const *awk_file;
			if (ii < (2*opt_cmd.size())) {	// For each cmd, the
											// /proc/<pid>/stat file 
				// will be referenced twice.
				if ((ii&1)==0) {
					read_proc_file( pidfile[ii/2].c_str(),proc_stats, sizeof(proc_stats) );
				}
				awk_in=proc_stats;   awk_file=NULL;
			} else if (spec2[ii][s_file] != prv_file) {
				prv_file = spec2[ii][s_file];
				read_proc_file( spec2[ii][s_file].c_str(), proc_stats, sizeof(proc_stats) );
				awk_in=proc_stats;   awk_file=NULL;
			}


			std::string stat_str=AWK( awkCmd[ii], awk_file, awk_in );

			long stat=atol(stat_str.c_str());

			if (spec2[ii][s_rate] == "yes") {
				double rate;
				if (stat_str!="\n")
					rate=(stat-pre_vals[ii])*multipliers[ii]/period;
				else
					rate=0.0;
				TRACE( 3, tmpdbg+"stat_str[0]=0x%x stat_str.size()=%zd", lp,ii,stat,rate, stat_str[0], stat_str.size() );
				fprintf(outfp, " %.2f",rate );
				if (rate < 0.0 && spec2[ii][s_file] == "/proc/diskstats") {
					TRACE( 0, "main stat:"+spec2[ii][s_desc]+" rate=%f pre_val=%ld stat=%ld stat_str=\""+stat_str\
					      +"\" awkCmd="+awkCmd[ii]+" proc_diskstats="+proc_stats
					      , rate, pre_vals[ii], stat );
					//TRACE_CNTL( "modeM", 0L );
				}
				pre_vals[ii] = stat;
			} else {
				TRACE( 3, "main lp=%d done stat%zd=%ld", lp, ii, stat );
				fprintf(outfp, " %.2f",stat*multipliers[ii] );
			}
		}
		fprintf(outfp,"\n"); fflush(outfp);
 eintr2:
		int64_t t_sleep=(t_start+(uint64_t)(period*lp*1000000))-gettimeofday_us();
		if (t_sleep > 0) {
			int sts=usleep( t_sleep );
			TRACE( 3,"main usleep sts=%d errno=%d",sts,errno );
			if(errno == EINTR)
				goto eintr2;
		}
		pp = check_pid_vec();
		TRACE( 2, "main pp=%d t_sleep=%ld", pp, t_sleep );
		if (pp == -1) {
			if (post_periods_completed == 0)
				TRACE( 1, "main processes complete - waiting %d post periods", opt_post );
			if (post_periods_completed++ == opt_post)
				break;
		}
	}
	if (lp==MAX_LP) {
		fprintf(outfp,"# MAX_LP abort\n" );
	}

	//TRACE( 0, "main waiting for pid=%d", pid );
	//wait(&status);
	//TRACE( 0, "main status=%d",status );
	TRACE( 0, "main done/complete/returning" );
	//TRACE_CNTL( "modeM", 0L );
	return (0);
}   // main
