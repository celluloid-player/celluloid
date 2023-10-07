# Reporting Bugs
- Make sure you're using the latest release.
- If possible, also check if the problem has already been fixed in git master.
- Don't report multiple unrelated/barely related bugs in one issue.
- Attach a log file.
	- You can make Celluloid print log messages by running
	  `G_MESSAGES_DEBUG=all celluloid --mpv-msg-level=all=trace`
	- The command above will cause Celluloid to generate a large amount of
	  log messages, so you may also want to redirect the output to a file.
	  For example, you can redirect log messages to `/tmp/celluloid.log`
	  with this command:
	  `G_MESSAGES_DEBUG=all celluloid --mpv-msg-level=all=trace > /tmp/celluloid.log`.
- If the issue involves crashing, include a backtrace in the report.
	- You can create a backtrace by running
	  `gdb -ex 'set pagination off' -ex run -ex bt celluloid`
	  and perform the action that triggers the crash.
- If the issue is a regression, it would be very helpful if you could perform a
  [bisect](https://git-scm.com/docs/git-bisect) to locate the commit that
  introduces the regression.
