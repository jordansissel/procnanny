# procnanny

A modern attempt to run processes and daemons.

## Goals

* Provide an API
* Be a state machine. Define transitions between each state.
* Don't get in the way.

## Why?

Supervisord has a number of issues. I could file bugs and feature requests and
hope they get fixed, or I could have more fun writing code.

Daemontools is excellent, but makes it hard to do more modern things (run N
instances of something, safely do nice/ionice/ulimit settings, etc)

Neither really present something programmable. I want events fired off when a
program crashes - poke my load balancer, fire an alert, signal a rebuild, etc.

I want a state machine that permits changes from any state to any state.

I want tracking of past behavior. Is this process flapping? Did it just die?
What was the last exit code and how long did the process run?

etc...

## Example client output

The RPC/API interface uses msgpack on top of zmq. This lets me write clients in most languages.

    % ruby client.rb status
    {
      "programs" => {
        "hello world" => {
          "command"   => "/usr/bin/env",
          "args"      => [
            [0] "ruby",
            [1] "-e",
            [2] "sleep(rand * 10); puts :OK"
          ],
          "uid"       => 500,
          "gid"       => 500,
          "nice"      => 1,
          "ionice"    => 0,
          "active"    => true,
          "instances" => {
            0 => {
              "pid"         => 8619,
              "state"       => "starting",
              "exitcode"    => 0,
              "exitsignal"  => 0,
              "admin_state" => "unknown"
            },
            1 => {
              "pid"         => 8621,
              "state"       => "starting",
              "exitcode"    => 0,
              "exitsignal"  => 0,
              "admin_state" => "unknown"
            },
            2 => {
              "pid"         => 8618,
              "state"       => "exited",
              "exitcode"    => 0,
              "exitsignal"  => 0,
              "admin_state" => "unknown"
            }
          }
        }
      }
    }

Example of an invalid request:

    % ruby client.rb fizzle
    {
      "error"   => "No such method requested",
      "request" => {
        "request" => "fizzle"
      }
    }

