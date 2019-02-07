#!/usr/bin/env python3
import os
import sys
import argparse
import subprocess


PROJECT_ROOT = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))
DOCKERFILE = os.path.abspath(os.path.join(os.path.dirname(__file__), 'Dockerfile.production.OEFNode'))
REMOTE_REPOSITORY = 'gcr.io/organic-storm-201412/oef-node'


def parse_commandline():
    parser = argparse.ArgumentParser()
    parser.add_argument('-p', '--publish', action='store_true', help='Push the image to the remote')
    return parser.parse_args()


def determine_project_version():
    cmd = ['git', 'describe', '--dirty=-wip', '--always']
    return subprocess.check_output(cmd, cwd=PROJECT_ROOT).decode().strip()


def main():
    args = parse_commandline()

    # determine the version
    version = determine_project_version()
    rebuildable = False if version.endswith('-wip') else True

    # define all the docker tags
    latest_tag = 'oef-node:latest'
    local_tag = 'oef-node:{}'.format(version)
    remote_tag = '{}:{}'.format(REMOTE_REPOSITORY, version)

    print('Version.....: {}'.format(version))
    print('Latest Tag..: {}'.format(latest_tag))
    print('Local Tag...: {}'.format(local_tag))
    print('Remote Tag..: {}'.format(remote_tag))
    print('Rebuilable..: {}'.format(rebuildable))

    # build the latest docker image
    cmd = [
        'docker',
        'build',
        '-t', local_tag,
        '-f', DOCKERFILE,
        PROJECT_ROOT,
    ]
    subprocess.check_call(cmd, cwd=PROJECT_ROOT)

    # tag the image as the latest
    cmd = [
        'docker',
        'tag',
        local_tag,
        latest_tag,
    ]
    subprocess.check_call(cmd)

    # publish the image if that is what is required
    if args.publish:

        # ensure that we have a rebuildable image
        if not rebuildable:
            print('The OEF node is not currently rebuildable, please commit first and retry')
            sys.exit(1)

        # make the remote tag
        cmd = [
            'docker',
            'tag',
            local_tag,
            remote_tag,
        ]
        subprocess.check_call(cmd)

        # push the remote tag
        cmd = [
            'docker',
            'push',
            remote_tag
        ]
        subprocess.check_call(cmd)


if __name__ == '__main__':
    main()
