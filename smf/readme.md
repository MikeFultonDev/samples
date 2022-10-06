# SMF records for sshd connection auditing

Sample way to run:
```
./getsmf119.sh | xargs ./smf119 | ./smfsshcorrelate.sh /tmp/prev /tmp/now >/tmp/connections.csv
```

Note the process leaves behind a temporary dataset called <HLQ>.SMF<tmpname>
After the pipeline has run, it can safely be deleted.

TBD: `/tmp/prev` file is not processed in any way.
