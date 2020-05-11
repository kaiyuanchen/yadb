# yadb, a RAFT-based distributed nosql database
	•	50000 writes per second
	•	RAFT consensus algorithm for managing a replicated log
	•	Provide simple query languages, e.g. INSERT {“key”:”123”, “val”:”str1”}

# lib required:  
```g++ 4.9  
c++ boost 1.55  
protobuf 2.5  
```

# install
```
mkdir builds && cd builds
cmake -Dtest=ON ..
```
