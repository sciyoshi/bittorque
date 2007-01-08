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

SConsignFile(os.path.join('.scons', 'sconsign'))

opts = Options('config', ARGUMENTS)

opts.AddOptions(
	BoolOption('enable_debug', 'Enable debug symbols', 1),
	EnumOption('enable_python', 'Enable building Python module', 'auto', allowed_values=('auto', 'yes', 'no'), ignorecase=1),
	PathOption('python_include_dir', 'Path for Python headers', '/usr/include/python2.4'),
	PathOption('data_dir', 'Path to search for images and other data', Dir('#/data').abspath))

env = Environment(options=opts, tools=['mingw', 'default', 'disttar'], toolpath=['tools'], ENV=os.environ)

env.Help(opts.GenerateHelpText(env))

env['DISTTAR_FORMAT'] = 'bz2'
env.Append(DISTTAR_EXCLUDEEXTS=['.o', '.so', '.pyc', '.a', '.so', '.bz2'], DISTTAR_EXCLUDEDIRS=['.scons'])

conf = env.Configure(
	conf_dir='.scons',
	log_file=os.path.join('.scons', 'log'),
	custom_tests={
		'CheckPKGConfig': check_pkgconfig,
		'CheckPKG': check_pkg})

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

if not conf.CheckPKG('gmodule-2.0', '2.12.0'):
	print 'gmodule-2.0 >= 2.12.0 not found'
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

have_pygtk = False

if env['enable_python'] != 'no' and conf.CheckPKG('pygtk-2.0'):
	have_pygtk = True

if env['enable_python'] == 'yes' and not have_pygtk:
	print 'pygtk-2.0 not found'
	Exit(1)

cpppath = env.get('CPPPATH')
env.Append(CPPPATH=['${python_include_dir}'])

if env['enable_python'] in ['yes', 'auto'] and have_pygtk:
	if not conf.CheckCHeader('Python.h') and env['enable_python'] == yes:
		print 'Python header not found. Try adding python-include-dir to the command line.'
		Exit(1)
	env['enable_python'] = 'yes'

if not have_pygtk:
	env.Replace(CPPPATH=cpppath)

have_gcrypt = False

if conf.CheckHeader('gcrypt.h'):
	have_gcrypt = True

env = conf.Finish()

env.ParseConfig('pkg-config --cflags --libs glib-2.0 gobject-2.0 gthread-2.0 gnet-2.0')
env.Append(CCFLAGS=['-Wall', '-Wextra'])

if env['enable_debug']:
	env.Append(CCFLAGS=['-g'])

env.Append(CPPDEFINES=['-DGNET_EXPERIMENTAL'])

if have_gcrypt:
	env.Append(CPPDEFINES=['-DHAVE_GCRYPT_H'])
	env.ParseConfig('libgcrypt-config --cflags --libs')

env.Append(CPPDEFINES=['-DBT_DATA_DIR=\\"' + env['data_dir'] + '\\"'])
env.Append(CPPPATH=[])

envlib = env.Copy()
envlib.Append(CPPDEFINES=['-DG_LOG_DOMAIN=\\"BitTorque\\"'])

envgtk = env.Copy()
envgtk.Append(LINKFLAGS=['-Wl,--export-dynamic'])
envgtk.Append(LIBPATH=['#/src/libbittorque'])
envgtk.Append(LIBS=['bittorque', 'gnet-2.0'])
envgtk.ParseConfig('pkg-config --cflags --libs gtk+-2.0 libglade-2.0 glib-2.0 gmodule-2.0 gnet-2.0')

if env['enable_python'] == 'yes':
	envpy = env.Copy()
	envpy.ParseConfig('pkg-config --cflags --libs gtk+-2.0 glib-2.0 gmodule-2.0 gnet-2.0 pygtk-2.0')
	envpy.Append(CPPPATH=['${python_include_dir}', '#/src/libbittorque'])
	envpy.Append(LIBPATH=['#/src/libbittorque'])
	envpy.Append(LIBS=['bittorque'])
else:
	envpy = None


#env.DistTar('bittorque', [env.Dir('#')])

Export('env', 'envlib', 'envgtk', 'envpy')

SConscript(['src/SConscript'])
