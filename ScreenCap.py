# ScreenCap.py
# 
# Author: Richard Barajas
# Date: 27-08-2012
#
# Purpose:
#   -Script invokes import command to take screenshot of desktop periodically on linux distro
#   for a requested amount of time.
#   -Use argparse module.
    

import argparse
import errno
import os
import sys
import time
import datetime

targetDir = "./"

parser = argparse.ArgumentParser()
parser.add_argument('-D', '--Directory', action='store', dest='directory', help='Directory/path to save images. Default is ./')
parser.add_argument('-d', '--Duration', action='store', dest='duration', required=True, type=int, help='Duration of runtime for script in seconds by default.')
parser.add_argument('-p', '--Period', action='store', dest='period', required=True, type=int, help='Interval between screenshots in seconds by default.')


def dirExists(target):
    return os.path.isdir(targetDir)

def main():
    global targetDir
    args = vars(parser.parse_args())
    if args['directory'] is not None:
        targetDir = args['directory']

    duration = args['duration']
    if duration < 0:
        print "Duration must be non-negative."
        sys.exit()

    period = args['period']
    if period <= 0:
        print "Period must be greater than zero."
        sys.exit()

    # Make sure proper directory name ending in slash
    if targetDir[-1] != '/':
        targetDir += '/' 
    targetDir += datetime.datetime.now().strftime("%Y-%m-%d") + "/"
    print "Images shall be stored under appropiate date folder in \'" + targetDir + "\'"

    if not dirExists(targetDir):
        try:
            os.makedirs(targetDir)
        except OSError as exception:
            if exception.errno != errno.EEXIST:
                raise

    periodicShots(duration, period)

def periodicShots(duration, period):
    """
    Allows program to capture screen in a timely manner

    dur: How long to take screenshots in seconds; 0 equal infinite duration time.
    per:   Time interval between screenshots in seconds.
    """
    start = now = time.time()
    while progressUpdate(start, now, duration) < 100 :
        snapShot()
        time.sleep(0.5)
        now = time.time()

    print

def progressUpdate(timeStarted, now, duration):
    """
    Presents visual progress via progress bar and percentage on stdout.
    Function returns percentage of time lapse from total time request.
    """
    timeDifference = now - timeStarted
    percentage = timeDifference / duration  * 100
    if percentage > 100:
        percentage = 100
    timeDifference = duration - timeDifference
    sys.stdout.write('\r')
    sys.stdout.write("[%-50s]\t%d%%" % ('=' * int(percentage / 2), percentage))
    sys.stdout.flush()
    return percentage

def snapShot():
    """
    Takes a screenshot of current desktop. Saves image with timestap as filename.
    """

    timeStamp = targetDir + datetime.datetime.now().strftime("%Y-%m-%d_%H%M%S")

    # import - saves  any visible window on an X server and outputs it as an 
    #      image file. You can capture a single window, the entire screen, 
    #      or any rectangular portion of the screen.
    command = "import -window root " + timeStamp + ".png"
    if os.system(command) != 0:
        # command failed
        sys.stderr.write(datetime.datetime.now().strftime("%Y-%m-%d_%H%M%S") + '\timport command failed.\n')
        sys.exit()

if __name__ == "__main__":
    main()

