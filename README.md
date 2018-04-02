# Counter-Incrementor
Counter Incrementor is a tool to troubleshoot and stress test POSIX file locking problems.

# Background Origin
Shortly after migrating Netapp data to Tegile data, 12 Oracle DBs crashed.  Oracle logs showed fatal IO errors.  Coinciding system logs showed NFS lock failures as follows.

kernel: NFS: nfs4_reclaim_open_state: Lock reclaim failed!

Support cases were opened with Tegile and Redhat.  The Tegile engineers said there were no hints or clues to the root cause within the Tegile NFS server logs.  Both Redhat and Tegile explained that network packet captures are key to finding and fixing the root cause; although, the problem can't be replicated and is unpredictable.

In come the Counter Incrementor.  It's sole purpose is to try making the problem show it's face whilst capturing network packets.  The program puts IO locking stress in Oracle DB fashion - POSIX locks - on a file that resides on a Tegile NFSv4 mount.

The Counter Incrementor will forks children and each child performs the following steps.

1. Acquire POSIX lock;
2. Read counter from file;
3. Increment counter;
4. Write counter to file;
5. Wait and hold lock for lock_delay seconds;
6. Release lock.

The program will be used to test NFSv4 locks in two different ways.  One way is a brute force method by spawning numerous kids that increment the counter numerous times as fast as possible.  The other way is to spawn few kids that idlely hold locks for lock_delay seconds in order to test NFSv4 lock leases.
