#
# Scons Output Colorizer v0.1 
# Copyright (C) 2008 Mihail 'IceBreaker' Szabolcs -- theicebreaker007@gmail.com
#
# Inspired from Blender's excellent SConstruct.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#

import os
import SCons

# shortcuts
Action = SCons.Action.Action
Builder = SCons.Builder.Builder
Tool = SCons.Tool

class colorizer:
	# define various colors
	cPurple	= '\033[95m'
	cBlue 	= '\033[94m'
	cGreen 	= '\033[92m'
	cYellow = '\033[93m'
	cRed 	= '\033[91m'
	cEnd 	= '\033[0m'

	def __init__(self,noColors=False):
		# create actions
		self.mylibaction 	= Action("$ARCOM", strfunction=self.my_linking_print)
		self.myshlibaction 	= Action("$SHLINKCOM", strfunction=self.my_linking_print_so)
		self.mylinkaction 	= Action("$LINKCOM", strfunction=self.my_program_print)
			
		self.mycaction 		= Action("$CCCOM", strfunction=self.my_compile_print)
		self.myshcaction 	= Action("$SHCCCOM", strfunction=self.my_compile_print)
		self.mycppaction 	= Action("$CXXCOM", strfunction=self.my_compile_print)
		self.myshcppaction 	= Action("$SHCXXCOM", strfunction=self.my_compile_print)
		
		# create builders
		self.shared_lib = Builder(	action = self.myshlibaction,
									emitter = '$SHLIBEMITTER',
									prefix = '$SHLIBPREFIX',
									suffix = '$SHLIBSUFFIX',
									src_suffix = '$SHOBJSUFFIX',
								   	src_builder = 'SharedObject' )

		self.static_lib = Builder(	action = self.mylibaction,
									emitter = '$LIBEMITTER',
									prefix = '$LIBPREFIX',
									suffix = '$LIBSUFFIX',
									src_suffix = '$OBJSUFFIX',
									src_builder = 'StaticObject' )

		self.program = Builder(	action = self.mylinkaction,
								emitter = '$PROGEMITTER',
								prefix = '$PROGPREFIX',
								suffix = '$PROGSUFFIX',
								src_suffix = '$OBJSUFFIX',
								src_builder = 'Object',
								target_scanner = SCons.Defaults.ProgScan )
								
		# just customize output, no colors
		if noColors:
			self.disableColors()
		
		return

	# custom 'output' functions
	def custom_print(self, c1, t1, c2, c3, t2):
		return c1+t1+self.cEnd+c2+" ==> "+self.cEnd+"'"+c3+"%s" % (t2) +self.cEnd+"'"+self.cEnd

	def my_compile_print(self, target, source, env):
		a = '%s' % (source[0])
		d, f = os.path.split(a)
		return self.custom_print(self.cBlue,"Compiling",self.cPurple,self.cYellow,f)
	
	def my_linking_print(self, target, source, env):
		t = '%s' % (target[0])
		d, f = os.path.split(t)
		return self.custom_print(self.cRed,"Linking library",self.cPurple,self.cYellow,f)

	def my_linking_print_so(self, target, source, env):
		t = '%s' % (target[0])
		d, f = os.path.split(t)
		return self.custom_print(self.cRed,"Linking shared object",self.cPurple,self.cYellow,f)

	def my_program_print(self, target, source, env):
		t = '%s' % (target[0])
		d, f = os.path.split(t)
		return self.custom_print(self.cRed,"Linking program",self.cPurple,self.cYellow,f)
		
	# set colors to empty strings thus disabling them on output
	def disableColors(self):
		self.cPurple	= ''
		self.cBlue 		= ''
		self.cGreen 	= ''
		self.cYellow	= ''
		self.cRed		= ''
		self.cEnd 		= ''
		return

	# activates the customizer on a given Scons Environment
	def colorize(self, env):
		if not env:
			return False

		# prepend/override the builders ( customize linking ) ...
		env.Prepend(BUILDERS={	'Program'		:self.program,
								'StaticLibrary' :self.static_lib,
								'SharedLibrary' :self.shared_lib })
		 
		# customize compiling ...
		static_ob, shared_ob = Tool.createObjBuilders(env)
		static_ob.add_action('.c', self.mycaction)
		static_ob.add_action('.cpp', self.mycppaction)
		shared_ob.add_action('.c', self.myshcaction)
		shared_ob.add_action('.cpp', self.myshcppaction)
		
		return True
