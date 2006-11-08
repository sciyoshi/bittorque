import os

#########################
## pkg-config checking ##
#########################

def check_pkgconfig(context, version):
	context.Message('Checking for pkg-config... ')
	ret = context.TryAction('pkg-config --atleast-pkgconfig-version=%s' % version)[0]
	context.Result(ret)
	return ret

def check_pkg(context, name, version=None):
	if version:
		context.Message('Checking for %s >= %s... ' % (name, version))
		ret = context.TryAction('pkg-config --atleast-version=%s \'%s\'' % (version, name))[0]
		context.Result(ret)
		return ret
	else:
		context.Message('Checking for %s... ' % name)
		ret = context.TryAction('pkg-config --exists \'%s\'' % name)[0]
		context.Result(ret)
		return ret


#################
## main script ##
#################

SConsignFile('.scons/sconsign')

env = Environment(tools=['default', 'disttar'], toolpath=['tools'], ENV=os.environ)

env['DISTTAR_FORMAT'] = 'bz2'
env.Append(DISTTAR_EXCLUDEEXTS=['.o', '.so', '.pyc', '.a', '.so', '.bz2'], DISTTAR_EXCLUDEDIRS=['.scons'])

conf = env.Configure(conf_dir='.scons', log_file='.scons/log', custom_tests={'CheckPKGConfig': check_pkgconfig, 'CheckPKG': check_pkg})

if not conf.CheckPKGConfig('0.15.0'):
	print 'pkg-config >= 0.15.0 not found'
	Exit(1)

if not conf.CheckPKG('glib-2.0', '2.12.0'):
	print 'glib-2.0 >= 2.12.0 not found'
	Exit(1)

if not conf.CheckPKG('gobject-2.0', '2.12.0'):
	print 'gobject-2.0 >= 2.12.0 not found'
	Exit(1)

if not conf.CheckPKG('gthread-2.0', '2.12.0'):
	print 'gthread-2.0 >= 2.12.0 not found'
	Exit(1)

if not conf.CheckPKG('gtk+-2.0', '2.10.0'):
	print 'gtk+-2.0 >= 2.10.0 not found'
	Exit(1)

if not conf.CheckPKG('libglade-2.0', '2.6.0'):
	print 'libglade-2.0 >= 2.6.0 not found'
	Exit(1)

if not conf.CheckPKG('gnet-2.0'):
	print 'gnet-2.0 not found'
	Exit(1)

env = conf.Finish()

env.ParseConfig('pkg-config --cflags --libs glib-2.0 gobject-2.0 gthread-2.0 gnet-2.0')
env.Append(CCFLAGS=['-g', '-Wall', '-Wextra', '-ansi'])
env.Append(CPPDEFINES=['-DGNET_EXPERIMENTAL'])
env.Append(CPPDEFINES=['-DDATADIR=\\"/home/sciyoshi/Build/bittorque/bittorque/data\\"'])
env.Append(CPPPATH=[])

envpy = env.Copy()
envpy.Append(LIBPATH=['#/src/bittorque'])
envpy.Append(LIBS=['bittorque'])
envpy.Append(CPPPATH=['/usr/include/python2.4'])
envpy.ParseConfig('pkg-config --cflags --libs pygobject-2.0')

envlib = env.Copy()
envlib.Append(CPPDEFINES=['-DG_LOG_DOMAIN=\\"Bittorque\\"'])

envgtk = env.Copy()
envgtk.Append(LINKFLAGS=['-Wl,--export-dynamic'])
envgtk.Append(LIBPATH=['#/src/bittorque'])
envgtk.Append(LIBS=['bittorque'])
envgtk.ParseConfig('pkg-config --cflags --libs gtk+-2.0 libglade-2.0 glib-2.0 gmodule-2.0 gnet-2.0')

#env.DistTar('bittorque', [env.Dir('#')])

Export('env', 'envlib', 'envgtk', 'envpy')

SConscript(['src/SConscript'])
