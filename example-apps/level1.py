#!/usr/bin/env python
# -*- coding: utf-8 -*-

import signal, time

class level1app:
    run = True
    save = False

    def sighandler(self, signum, frame):
        if signum == signal.SIGUSR1:
            self.save = True
        elif signum == signal.SIGTERM:
            self.run = False
        elif signum == signal.SIGINT:
            self.run = False
        else:
            print "Unknown signal %d received" % signum

    def run(self):
        signal.signal(signal.SIGUSR1, self.sighandler)
        signal.signal(signal.SIGTERM, self.sighandler)
        signal.signal(signal.SIGINT, self.sighandler)

        print "Waiting for signals..."

        while self.run:
            if self.save:
                print "Save"
                self.save = False
            time.sleep(0.1)

        print "Exit"

level1app().run()
