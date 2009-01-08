#!/usr/bin/python
#
################################################################################
# Dragoon - Copyright (C) 2008 - Michael Levin
#
# This program is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free Software
# Foundation; either version 2, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
################################################################################

import glob, os, sys
from colorizer import colorizer

# Package parameters
package = 'dragoon'
version = '0.0.0'

# Builder for precompiled headers
gch_builder = Builder(action = '$CC $CFLAGS $CCFLAGS $_CCCOMCOM ' +
                               '-x c-header -c $SOURCE -o $TARGET')

# Convert a POSIX path to OS-indepentant (stub)
def path(p):
        return p

# Create a default environment. Have to set environment variables after
# initialization so that SCons doesn't mess them up.
default_env = Environment(ENV = os.environ, BUILDERS = {'GCH' : gch_builder})

# Tells SCons to be smarter about caching dependencies
default_env.SetOption('implicit_cache', 1)

# Adds an option that may have been set via environment flags
def AddEnvOption(env, opts, opt_name, opt_desc, env_name = None, value = None):
        if env_name == None:
                env_name = opt_name;
        if env_name == opt_name:
                opt_desc += ' (environment variable)'
        else:
                opt_desc += ' (environment variable ' + env_name + ')'
        if env_name in os.environ:
                opts.Add(opt_name, opt_desc, os.environ[env_name])
        elif value == None:
                opts.Add(opt_name, opt_desc, env.get(opt_name))
        else:
                opts.Add(opt_name, opt_desc, value)

# Options are loaded from 'custom.py'. Default values are only provided for
# the variables that do not get set by SCons itself.
config_file = ARGUMENTS.get('config', 'custom.py')
opts = Options(config_file)

# Define and load the options
AddEnvOption(default_env, opts, 'CC', 'Compiler to use')
AddEnvOption(default_env, opts, 'CFLAGS', 'Compiler flags')
AddEnvOption(default_env, opts, 'LINK', 'Linker to use')
AddEnvOption(default_env, opts, 'LINKFLAGS', 'Linker flags', 'LDFLAGS')
AddEnvOption(default_env, opts, 'PREFIX', 'Installation path prefix',
             'PREFIX', '/usr/local/')
opts.Add(BoolOption('pch', 'Use precompiled headers if possible', True))
opts.Add(BoolOption('checked', 'Enable debug checks', False))
opts.Add(BoolOption('color', 'Colored SCons output', True))
opts.Add(BoolOption('dump_env', 'Dump SCons Environment contents', False))
opts.Add(BoolOption('dump_path', 'Dump path enviornment variable', False))
opts.Update(default_env)
opts.Save(config_file, default_env)

# Convert output spam to tidy colored lines
if default_env['color']:
        col = colorizer()
        col.colorize(default_env)

# Dump Environment object to debug SCons
if default_env['dump_env']:
        print default_env.Dump()
if default_env['dump_path']:
        print default_env['ENV']['PATH']

# Setup the help information
Help('\n' + package.title() + ' ' + version +
""" is built using SCons. See the README file for detailed
instructions.

Options are initialized from the environment first and can then be overridden
using either the command-line or the configuration file. Values only need to
be specified on the command line once and are then saved to the configuration
file. The filename of the configuration file can be specified using the
'config' command-line option:

  scons config='custom.py'

The following options are available:
""")
Help(opts.GenerateHelpText(default_env))

################################################################################
#
# scons -- Compile the game
#
################################################################################
game_src = (['src/main.c'] + glob.glob('src/*/*.c'))
game_env = default_env.Clone()
game_env.Append(CPPPATH = '.')
game_env.Append(LIBS = ['SDL_ttf', 'GL', 'GLU', 'png'])
game_env.ParseConfig('sdl-config --cflags --libs')
game_obj = game_env.Object(game_src)
game = game_env.Program(package, game_obj)
Default(game)

# Build the precompiled headers as dependencies of the main program. We have
# to manually specify the header dependencies because SCons is too stupid to
# scan the header files on its own.
game_pch = []
def GamePCH(header, deps = []):
        global game_pch
        pch = game_env.GCH(header + '.gch', header)
        game_env.Depends(pch, deps)
        game_env.Depends(game_obj, pch)
        game_pch += pch
if game_env['pch']:
        common_deps = glob.glob('src/common/*.h')
        GamePCH('src/common/c_public.h', common_deps)

# Generate a config.h with definitions
def WriteConfigH(target, source, env):
        config = open('config.h', 'w')
        config.write('/* Package parameters */\n' +
                     '#define PACKAGE "' + package + '"\n' +
                     '#define PACKAGE_STRING "' + package.title() +
                     ' ' + version + '"\n' +
                     '#define PKGDATADIR "' + install_data + '"\n' +
                     '\n/* Compilation parameters */\n' +
                     '#define CHECKED %d\n' % (game_env['checked']));
        config.close()

game_config = game_env.Command('config.h', '', WriteConfigH)
game_env.Depends(game_config, 'SConstruct')
game_env.Depends(game_obj + game_pch, game_config)
game_env.Depends(game_config, config_file)

################################################################################
#
# scons gendoc -- Generate automatic documentation
#
################################################################################
gendoc_env = default_env.Clone()
gendoc_header = path('gendoc/header.html')
gendoc = gendoc_env.Program(path('gendoc/gendoc'),
                            glob.glob(path('gendoc/*.c')))

def GendocOutput(output, in_path, title, inputs = []):
        output = os.path.join('gendoc', 'docs', output)
        inputs = (glob.glob(os.path.join(in_path, '*.h')) +
                  glob.glob(os.path.join(in_path, '*.c')) + inputs);
        cmd = gendoc_env.Command(output, inputs, path('gendoc/gendoc') +
                                 ' --file $TARGET --header "' +
                                 gendoc_header + '" --title "' + title + '" ' +
                                 ' '.join(inputs))
        gendoc_env.Depends(cmd, gendoc)
        gendoc_env.Depends(cmd, gendoc_header)

# Gendoc HTML files
GendocOutput('gendoc.html', 'gendoc', 'GenDoc')
GendocOutput('shared.html', path('src/common'), package.title() + ' Shared',
             [path('src/render/r_public.h'),
              path('src/physics/p_public.h'),
              path('src/game/g_public.h'),] +
             glob.glob(path('src/render/*.c')) +
             glob.glob(path('src/physics/*.c')) +
             glob.glob(path('src/game/*.c')))
GendocOutput('render.html', path('src/render'), package.title() + ' Render')
GendocOutput('game.html', path('src/game'), package.title() + ' Game')
GendocOutput('physics.html', path('src/physics'),
             package.title() + ' Physics')

################################################################################
#
# scons install -- Install the game
#
################################################################################
install_prefix = os.path.join(os.getcwd(), game_env['PREFIX'])
install_data = os.path.join(install_prefix, 'share', package)
default_env.Alias('install', install_prefix)
default_env.Depends('install', game)

def InstallRecursive(dest, src, exclude = []):
        bad_exts = ['exe', 'o', 'obj', 'gch', 'pch', 'user', 'ncb', 'suo',
                    'manifest']
        files = os.listdir(src)
        for f in files:
                ext = f.rsplit('.')
                ext = ext[len(ext) - 1]
                if (f[0] == '.' or ext in bad_exts):
                        continue
                src_path = os.path.join(src, f)
                if src_path in exclude:
                        continue
                if (os.path.isdir(src_path)):
                        InstallRecursive(os.path.join(dest, f), src_path)
                        continue
                default_env.Install(dest, src_path)

# Files that get installed
default_env.Install(os.path.join(install_prefix, 'bin'), game)
default_env.Install(install_data, ['COPYING', 'README'])
InstallRecursive(os.path.join(install_data, 'game'), 'game')
InstallRecursive(os.path.join(install_data, 'gfx'), 'gfx')
InstallRecursive(os.path.join(install_data, 'map'), 'map')
InstallRecursive(os.path.join(install_data, 'sfx'), 'sfx')

################################################################################
#
# scons dist -- Distributing the source package tarball
#
################################################################################
dist_name = package + '-' + version
dist_tarball = dist_name + '.tar.gz'
default_env.Command(dist_tarball, dist_name,
                    ['tar -czf' + dist_tarball + ' ' + dist_name])
default_env.Alias('dist', dist_tarball)
default_env.AddPostAction(dist_tarball, Delete(dist_name))

# Files that get distributed in a source package
default_env.Install(dist_name, ['COPYING', 'README', 'SConstruct', 'Makefile'])
InstallRecursive(os.path.join(dist_name, 'game'), 'game')
InstallRecursive(os.path.join(dist_name, 'gfx'), 'gfx')
InstallRecursive(os.path.join(dist_name, 'map'), 'map')
InstallRecursive(os.path.join(dist_name, 'sfx'), 'sfx')
InstallRecursive(os.path.join(dist_name, 'src'), 'src')

