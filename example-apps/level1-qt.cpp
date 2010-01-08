/* Basic code to enable LADISH level 1 support to an event driven Qt (3/4) 
 * application
 * (c) 2010 Guido Scholz, License: GNU GPL v2
 */


/* (1) */
/* Add this declaration to the header file of your MainWindow class:*/

private:
    static int sigpipe[2];

    static void handleSignal(int);
    bool installSignalHandlers();

private slots:
    void signalAction(int);



/* (2) */
/* Add this code to the cpp file of your MainWindow class:*/

#include <cerrno>   // for errno
#include <csignal>  // for sigaction()
#include <cstring>  // for strerror()
#include <unistd.h> // for pipe()
#include <QSocketNotifier>


int MainWindow::sigpipe[2];


MainWindow::MainWindow(...)
{
    /* ... */
    /*add this lines to the constuctor of your MainWindow class: */
    if (!installSignalHandlers())
        qWarning("%s", "Signal handlers not installed!");
    /* ... */
}



/* Handler for system signals (SIGUSR1, SIGINT...)
 * Write a message to the pipe and leave as soon as possible
 */
void MainWindow::handleSignal(int sig)
{
    if (write(sigpipe[1], &sig, sizeof(sig)) == -1) {
        qWarning("write() failed: %s", std::strerror(errno));
    }
}

/* Install signal handlers (may be more than one; called from the
 * constructor of your MainWindow class*/
bool MainWindow::installSignalHandlers()
{
    /*install pipe to forward received system signals*/
    if (pipe(sigpipe) < 0) {
        qWarning("pipe() failed: %s", std::strerror(errno));
        return false;
    }

    /*install notifier to handle pipe messages*/
    QSocketNotifier* signalNotifier = new QSocketNotifier(sigpipe[0],
            QSocketNotifier::Read, this);
    connect(signalNotifier, SIGNAL(activated(int)),
            this, SLOT(signalAction(int)));

    /*install signal handlers*/
    struct sigaction action;
    memset(&action, 0, sizeof(action));
    action.sa_handler = handleSignal;

    if (sigaction(SIGUSR1, &action, NULL) == -1) {
        qWarning("sigaction() failed: %s", std::strerror(errno));
        return false;
    }

    /* optional: register more signals to handle:
    if (sigaction(SIGINT, &action, NULL) == -1) {
        qWarning("sigaction() failed: %s", std::strerror(errno));
        return false;
    }
     */

    return true;
}

/* Slot to give response to the incoming pipe message;
   e.g.: save current file */
void MainWindow::signalAction(int fd)
{
    int message;

    if (read(fd, &message, sizeof(message)) == -1) {
        qWarning("read() failed: %s", std::strerror(errno));
        return;
    }
    
    switch (message) {
        case SIGUSR1:
            saveFile();
            break;
    /* optional: handle more signals:
        case SIGINT:
            //do something usefull like ...
            close();
            break;
     */
        default:
            qWarning("Unexpected signal received: %d", message);
            break;
    }
}

