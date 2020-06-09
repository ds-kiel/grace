# IoT-Testbed

A very simple testbed for the IoT!

# Orchestration Server Development

Build docker image:
```
    docker build . -t testbed
```


Run docker image interactively
```
    docker run -v $PWD/server/:/usr/testbed/ --add-host="raspi01:192.168.1.111" -ti testbed
```

A user `developer` is initially created that is allowed to run the testbed script.

Some manual steps have to be done:
Before using the `tesbed.py` script a ssh-key has to be generated for the user `developer`. (It is
advisable to generate the ssh-keys outside of docker and link them into the docker container using the
`--volume` flag).

Add the public key information of the generated ssh key to all raspberry pi's `~/.ssh/authorized-keys`.

Finally run ssh-keyscan for all participating raspberry pi's
```
for ip in $(cat /usr/testbed/scripts/all-hosts); do
  if [ -z `ssh-keygen -F $ip` ]; then
    ssh-keyscan -H $ip >> ~/.ssh/known_hosts
  fi
done
```

