from contextlib import contextmanager
import inspect
import multiprocessing
import os
import shutil
import subprocess
import sys
import urllib


################################################################################
#
# Shared configuration
#
################################################################################


def join_flags(*flags):
    return ' '.join(flags).strip()


building_for_ios = False
if os.environ['PLATFORM_NAME'] in ('iphoneos', 'iphonesimulator'):
    building_for_ios = True
else:
    assert os.environ['PLATFORM_NAME'] == 'macosx'

assert os.environ['GCC_VERSION'] == 'com.apple.compilers.llvm.clang.1_0'
ar = os.environ['DT_TOOLCHAIN_DIR'] + '/usr/bin/ar'
cc = os.environ['DT_TOOLCHAIN_DIR'] + '/usr/bin/clang'
cxx = os.environ['DT_TOOLCHAIN_DIR'] + '/usr/bin/clang++'
make = os.environ['DEVELOPER_DIR'] + '/usr/bin/make'
rsync = '/usr/bin/rsync'
xcodebuild = os.environ['DEVELOPER_DIR'] + '/usr/bin/xcodebuild'

num_cores = str(multiprocessing.cpu_count())

common_flags = ' '.join(('-arch ' + arch) for arch in
                        os.environ['ARCHS'].split())
common_flags += ' -isysroot %(SDKROOT)s'
if building_for_ios:
    common_flags += ' -miphoneos-version-min=%(IPHONEOS_DEPLOYMENT_TARGET)s'
else:
    common_flags += ' -mmacosx-version-min=%(MACOSX_DEPLOYMENT_TARGET)s'
common_flags %= os.environ

compile_flags = ('-g -Os -fexceptions -fvisibility=hidden ' +
                 '-Werror=unguarded-availability ' +
                 common_flags)
if os.environ['ENABLE_BITCODE'] == 'YES':
    compile_flags += {
        'bitcode': ' -fembed-bitcode',
        'marker': ' -fembed-bitcode-marker',
        }.get(os.environ['BITCODE_GENERATION_MODE'], '')

cflags = '-std=%(GCC_C_LANGUAGE_STANDARD)s' % os.environ
cxxflags = ('-std=%(CLANG_CXX_LANGUAGE_STANDARD)s '
            '-stdlib=%(CLANG_CXX_LIBRARY)s'
            % os.environ)

link_flags = common_flags

downloaddir = os.path.abspath('download')
patchdir = os.path.abspath('patches')
xcconfigdir = os.path.abspath('../build/xcode')
builddir = os.environ['TARGET_TEMP_DIR']

prefix = os.environ['BUILT_PRODUCTS_DIR']
frameworksdir = prefix + '/Frameworks'
matlabdir = prefix + '/MATLAB'
includedir = prefix + '/include'
libdir = prefix + '/lib'


################################################################################
#
# Build helpers
#
################################################################################


all_builders = []


def builder(func):
    argspec = inspect.getargspec(func)
    defaults = dict(zip(argspec[0], argspec[3] or []))
    if building_for_ios:
        if defaults.get('ios', False):
            all_builders.append(func)
    else:
        if defaults.get('macos', True):
            all_builders.append(func)
    return func


def announce(msg, *args):
    sys.stderr.write((msg + '\n') % args)


def check_call(args, **kwargs):
    announce('Running command: %s', ' '.join(repr(a) for a in args))
    cmd = subprocess.Popen(args,
                           stdout = subprocess.PIPE,
                           stderr = subprocess.STDOUT,
                           **kwargs)
    output = cmd.communicate()[0]
    if 0 != cmd.returncode:
        announce('Command exited with status %d and output:\n%s',
                 cmd.returncode,
                 output)
        sys.exit(1)


@contextmanager
def workdir(path):
    old_path = os.getcwd()
    announce('Entering directory %r', path)
    os.chdir(path)
    yield
    announce('Leaving directory %r', path)
    os.chdir(old_path)


class DoneFileExists(Exception):
    pass


@contextmanager
def done_file(tag):
    filename = tag + '.done'
    if os.path.isfile(filename):
        raise DoneFileExists
    yield
    with open(filename, 'w') as fp:
        fp.write('Done!\n')


def always_download_file(url, filepath):
    check_call(['/usr/bin/curl', '-#', '-L', '-f', '-o', filepath, url])


def download_file(url, filename):
    filepath = downloaddir + '/' + filename
    if os.path.isfile(filepath):
        announce('Already downloaded file %r', filename)
    else:
        always_download_file(url, filepath)


def download_archive(url_path, filename):
    download_file(url_path + filename, filename)


def download_archive_from_sf(path, version, filename):
    url = (('http://downloads.sourceforge.net/project/%s/%s/%s'
            '?use_mirror=autoselect') % (path, version, filename))
    return download_file(url, filename)


def make_directory(path):
    if not os.path.isdir(path):
        check_call(['/bin/mkdir', '-p', path])


def make_directories(*args):
    for path in args:
        make_directory(path)


def remove_directory(path):
    if os.path.isdir(path):
        check_call(['/bin/rm', '-Rf', path])


def remove_directories(*args):
    for path in args:
        remove_directory(path)


def unpack_tarfile(filename, outputdir):
    check_call(['/usr/bin/tar', 'xf', downloaddir + '/' + filename])


def unpack_zipfile(filename, outputdir):
    check_call([
        '/usr/bin/unzip',
        '-q',
        downloaddir + '/' + filename,
        '-d', outputdir,
        ])


def apply_patch(patchfile, strip=1):
    with open(patchdir + '/' + patchfile) as fp:
        check_call(
            args = ['/usr/bin/patch', '-p%d' % strip],
            stdin = fp,
            )


def get_clean_env():
    env = os.environ.copy()

    # The presence of these can break some build tools
    env.pop('IPHONEOS_DEPLOYMENT_TARGET', None)
    env.pop('MACOSX_DEPLOYMENT_TARGET', None)
    if building_for_ios:
        env.pop('SDKROOT', None)

    return env


def run_b2(libraries, clean=False):
    b2_args = [
        './b2',
        #'-d', '2',  # Show actual commands run,
        '-j', num_cores,
        '--prefix=' + prefix,
        '--includedir=' + includedir,
        '--libdir=' + libdir,
        'variant=release',
        'optimization=space',
        'debug-symbols=on',
        'link=static',
        'threading=multi',
        'define=boost=mworks_boost',
        # Unfortunately, Boost.Python won't compile against Python 3.2+ with
        # Py_LIMITED_API defined
        #'define=Py_LIMITED_API',
        'cflags=' + compile_flags,
        'cxxflags=' + cxxflags,
        'linkflags=' + link_flags,
        ]
    b2_args += ['--with-' + l for l in libraries]
    if clean:
        b2_args.append('--clean')
    else:
        b2_args.append('install')
    check_call(b2_args)


def get_updated_env(
    extra_compile_flags = '',
    extra_cflags = '',
    extra_cxxflags = '',
    extra_ldflags = '',
    extra_cppflags = '',
    ):

    env = get_clean_env()
    env.update({
        'CC': cc,
        'CXX': cxx,
        'CFLAGS': join_flags(compile_flags, extra_compile_flags,
                             cflags, extra_cflags),
        'CXXFLAGS': join_flags(compile_flags, extra_compile_flags,
                               cxxflags, extra_cxxflags),
        'LDFLAGS': join_flags(link_flags, extra_ldflags),
        'CPPFLAGS': join_flags(common_flags, extra_cppflags),
        })
    return env


def run_make(targets=[]):
    check_call([make, '-j', num_cores] + targets)


def run_configure_and_make(
    extra_args = [],
    command = ['./configure'],
    extra_compile_flags = '',
    extra_cflags = '',
    extra_cxxflags = '',
    extra_ldflags = '',
    extra_cppflags = '',
    ios_host_platform = 'arm-apple-darwin',
    ):

    args = [
        '--prefix=' + prefix,
        '--includedir=' + includedir,
        '--libdir=' + libdir,
        '--disable-dependency-tracking',
        '--disable-shared',
        '--enable-static',
        ]

    # Force configure into cross-compilation mode when building for an
    # iOS device or simulator
    if building_for_ios:
        args.append('--host=' + ios_host_platform)

    check_call(
        args = command + args + extra_args,
        env = get_updated_env(extra_compile_flags,
                              extra_cflags,
                              extra_cxxflags,
                              extra_ldflags,
                              extra_cppflags),
        )
    
    run_make(['install'])


def add_object_files_to_libpythonall(exclude=()):
    object_files = []
    for dirpath, dirnames, filenames in os.walk('.'):
        for name in filenames:
            if name.endswith('.o') and name not in exclude:
                object_files.append(os.path.join(dirpath, name))
    check_call([
        ar,
        'rcs',
        libdir + ('/libpython%s_all.a' % os.environ['MW_PYTHON_3_VERSION']),
        ] + object_files)


################################################################################
#
# Library builders
#
################################################################################


@builder
def libffi(ios=True):
    version = '3.3-rc0'
    srcdir = 'libffi-' + version
    tarfile = srcdir + '.tar.gz'

    with done_file(srcdir):
        if not os.path.isdir(srcdir):
            download_archive('https://github.com/libffi/libffi/releases/download/v%s/' % version, tarfile)
            unpack_tarfile(tarfile, srcdir)

        with workdir(srcdir):
            other_kwargs = {}
            if building_for_ios:
                assert os.environ['ARCHS'] == 'arm64'
                other_kwargs['ios_host_platform'] = 'aarch64-apple-darwin'
            run_configure_and_make(
                extra_args = ['--enable-portable-binary'],
                extra_cflags = '-std=gnu11',
                **other_kwargs
                )


@builder
def openssl(ios=True):
    version = '1.1.1c'
    srcdir = 'openssl-' + version
    tarfile = srcdir + '.tar.gz'

    with done_file(srcdir):
        if not os.path.isdir(srcdir):
            download_archive('https://www.openssl.org/source/', tarfile)
            unpack_tarfile(tarfile, srcdir)

        with workdir(srcdir):
            if building_for_ios:
                assert os.environ['ARCHS'] == 'arm64'
                config_name = 'ios64-cross'
            else:
                assert os.environ['ARCHS'] == 'x86_64'
                config_name = 'darwin64-x86_64-cc'

            env = get_clean_env()
            env['AR'] = ar
            env['CC'] = cc

            check_call([
                './Configure',
                config_name,
                '--prefix=' + prefix,
                'no-shared',
                join_flags(compile_flags, cflags, '-std=gnu11'),
                ],
                env = env)

            run_make()
            run_make(['install_sw'])


@builder
def python(ios=True):
    version = '3.7.4'
    srcdir = 'Python-' + version
    tarfile = srcdir + '.tgz'

    assert version[:version.rfind('.')] == os.environ['MW_PYTHON_3_VERSION']

    with done_file(srcdir):
        if not os.path.isdir(srcdir):
            download_archive('https://www.python.org/ftp/python/%s/' % version, tarfile)
            unpack_tarfile(tarfile, srcdir)
            with workdir(srcdir):
                apply_patch('python_ctypes.patch')
                apply_patch('python_static_zlib.patch')
                if building_for_ios:
                    apply_patch('python_ios_build.patch')
                    apply_patch('python_ios_disabled_modules.patch')
                    apply_patch('python_ios_fixes.patch')
                    apply_patch('python_ios_test_fixes.patch')
                else:
                    apply_patch('python_macos_10_13_required.patch')
                    apply_patch('python_macos_disabled_modules.patch')
                    apply_patch('python_macos_test_fixes.patch')

        with workdir(srcdir):
            extra_args = [
                '--without-ensurepip',
                '--with-openssl=' + prefix,
                ]
            if building_for_ios:
                extra_args += [
                    '--build=x86_64-apple-darwin',
                    '--enable-ipv6',
                    'PYTHON_FOR_BUILD=' + os.environ['MW_PYTHON_3'],
                    'ac_cv_file__dev_ptmx=no',
                    'ac_cv_file__dev_ptc=no',
                    ]
            else:
                # Set MACOSX_DEPLOYMENT_TARGET, so that the correct value is
                # recorded in the installed sysconfig data
                extra_args.append('MACOSX_DEPLOYMENT_TARGET=' +
                                  os.environ['MACOSX_DEPLOYMENT_TARGET'])

            run_configure_and_make(
                extra_args = extra_args,
                extra_compile_flags = '-fvisibility=default',
                )

            add_object_files_to_libpythonall(
                exclude = ['_testembed.o', 'python.o']
                )

            # Generate list of trusted root certificates (for ssl module)
            always_download_file(
                url = 'https://mkcert.org/generate/',
                filepath = os.path.join(os.environ['MW_PYTHON_3_STDLIB_DIR'],
                                        'cacert.pem'),
                )


@builder
def numpy(ios=True):
    version = '1.17.1'
    srcdir = 'numpy-' + version
    tarfile = srcdir + '.tar.gz'

    with done_file(srcdir):
        if not os.path.isdir(srcdir):
            download_archive('https://github.com/numpy/numpy/releases/download/v%s/' % version, tarfile)
            unpack_tarfile(tarfile, srcdir)
            with workdir(srcdir):
                if building_for_ios:
                    apply_patch('numpy_ios_fixes.patch')
                    apply_patch('numpy_ios_test_fixes.patch')

        with workdir(srcdir):
            env = get_clean_env()
            env['PYTHONPATH'] = os.environ['MW_PYTHON_3_STDLIB_DIR']

            # Don't use Accelerate, as it seems to make things worse rather
            # than better
            env['NPY_BLAS_ORDER'] = ''
            env['NPY_LAPACK_ORDER'] = ''

            if building_for_ios:
                env.update({
                    '_PYTHON_HOST_PLATFORM': 'darwin-arm',
                    '_PYTHON_SYSCONFIGDATA_NAME': '_sysconfigdata_m_darwin_darwin',
                    # numpy's configuration tests link test executuables using
                    # bare cc (without cflags).  Add common_flags to ensure that
                    # linking uses the correct architecture and SDK.
                    'CC': join_flags(cc, common_flags)
                    })

            check_call([
                os.environ['MW_PYTHON_3'],
                'setup.py',
                'build',
                '-j', num_cores,
                'install',
                '--prefix=' + prefix,
                # Force egg info in to a separate directory.  (Not sure why
                # including --root has this affect, but whatever.)
                '--root=/',
                ],
                env = env)

            add_object_files_to_libpythonall()

        # The numpy test suite requires pytest, so install it and its
        # dependencies (but outside of any standard location, because we
        # don't want to distribute it)
        check_call([
            os.environ['MW_PYTHON_3'],
            '-m', 'pip',
            'install',
            '--target', os.path.join(prefix, 'pytest'),
            'pytest',
            ])


@builder
def boost(ios=True):
    version = '1.71.0'
    srcdir = 'boost_' + version.replace('.', '_')
    tarfile = srcdir + '.tar.bz2'

    with done_file(srcdir):
        project_config_jam = 'project-config.jam'
        project_config_jam_orig = project_config_jam + '.orig'

        if not os.path.isdir(srcdir):
            download_archive('https://dl.bintray.com/boostorg/release/%s/source/' % version, tarfile)
            unpack_tarfile(tarfile, srcdir)
            with workdir(srcdir):
                os.symlink('boost', 'mworks_boost')
                env = get_clean_env()
                if building_for_ios:
                    # Need to use the macOS SDK when compiling the build system
                    env['SDKROOT'] = subprocess.check_output([
                        '/usr/bin/xcrun',
                        '--sdk', 'macosx',
                        '--show-sdk-path',
                        ]).strip()
                check_call([
                    './bootstrap.sh',
                    '--with-toolset=clang',
                    '--without-icu',
                    '--without-libraries=python',  # Configure python manually
                    ],
                    env = env,
                    )
                shutil.move(project_config_jam, project_config_jam_orig)
            
        with workdir(srcdir):
            shutil.copy(project_config_jam_orig, project_config_jam)
            libraries = ['filesystem', 'random', 'regex', 'thread']
            if not building_for_ios:
                libraries += ['serialization']
            run_b2(libraries)

            for tag in (() if building_for_ios else ('',)) + ('_3',):
                shutil.copy(project_config_jam_orig, project_config_jam)
                with open(project_config_jam, 'a') as fp:
                    fp.write('\nusing python : %s : %s : %s : %s ;\n' %
                             (os.environ['MW_PYTHON%s_VERSION' % tag],
                              # Prevent Boost's build system from running the
                              # Python executable
                              '/usr/bin/false',
                              os.environ['MW_PYTHON%s_INCLUDEDIR' % tag],
                              os.environ['MW_PYTHON%s_LIBDIR' % tag]))
                libraries = ['python']
                # Remove previous build products before building again
                run_b2(libraries, clean=True)
                run_b2(libraries)

        with workdir(includedir):
            if not os.path.islink('mworks_boost'):
                os.symlink('boost', 'mworks_boost')


@builder
def zeromq(ios=True):
    version = '4.3.2'
    srcdir = 'zeromq-' + version
    tarfile = srcdir + '.tar.gz'

    with done_file(srcdir):
        if not os.path.isdir(srcdir):
            download_archive('https://github.com/zeromq/libzmq/releases/download/v%s/' % version, tarfile)
            unpack_tarfile(tarfile, srcdir)

        with workdir(srcdir):
            run_configure_and_make(
                extra_args = [
                    '--disable-silent-rules',
                    '--disable-perf',
                    '--disable-curve-keygen',
                    '--disable-curve',
                    ],
                extra_ldflags = '-lc++',
                )


@builder
def msgpack(ios=True):
    version = '3.2.0'
    srcdir = 'msgpack-' + version
    tarfile = srcdir + '.tar.gz'

    with done_file(srcdir):
        if not os.path.isdir(srcdir):
            download_archive('https://github.com/msgpack/msgpack-c/releases/download/cpp-%s/' % version, tarfile)
            unpack_tarfile(tarfile, srcdir)

        with workdir(srcdir):
            check_call([rsync, '-a', 'include/', includedir])


@builder
def libxslt(macos=False, ios=True):
    version = '1.1.29'
    srcdir = 'libxslt-' + version
    tarfile = srcdir + '.tar.gz'

    with done_file(srcdir):
        if not os.path.isdir(srcdir):
            download_archive('ftp://xmlsoft.org/libxslt/', tarfile)
            unpack_tarfile(tarfile, srcdir)

        with workdir(srcdir):
            run_configure_and_make(
                extra_args = [
                    '--disable-silent-rules',
                    '--without-python',
                    '--without-crypto',
                    '--with-libxml-include-prefix=%(SDKROOT)s/usr/include/libxml2' % os.environ,
                    '--with-libxml-libs-prefix=%(SDKROOT)s/usr/lib' % os.environ,
                    '--without-plugins',
                    ],
                )


@builder
def sqlite(ios=True):
    version = '3290000'
    srcdir = 'sqlite-autoconf-' + version
    tarfile = srcdir + '.tar.gz'

    with done_file(srcdir):
        if not os.path.isdir(srcdir):
            download_archive('https://sqlite.org/2019/', tarfile)
            unpack_tarfile(tarfile, srcdir)

        with workdir(srcdir):
            extra_compile_flags = '-DSQLITE_DQS=0'  # Recommended as of 3.29.0
            if building_for_ios:
                extra_compile_flags = join_flags(extra_compile_flags,
                                                 '-DSQLITE_NOHAVE_SYSTEM')
            run_configure_and_make(
                extra_compile_flags = extra_compile_flags,
                )


@builder
def cppunit():
    version = '1.14.0'
    srcdir = 'cppunit-' + version
    tarfile = srcdir + '.tar.gz'

    with done_file(srcdir):
        if not os.path.isdir(srcdir):
            download_archive('http://dev-www.libreoffice.org/src/', tarfile)
            unpack_tarfile(tarfile, srcdir)

        with workdir(srcdir):
            run_configure_and_make(
                extra_compile_flags = '-g0',
                )


@builder
def matlab_xunit():
    version = '4.1.0'
    tag = 'matlab-xunit-'
    srcdir = tag * 2 + version
    tarfile = tag + version + '.tar.gz'

    with done_file(srcdir):
        if not os.path.isdir(srcdir):
            download_archive('https://github.com/psexton/matlab-xunit/archive/', tarfile)
            unpack_tarfile(tarfile, srcdir)

        with workdir(srcdir):
            make_directory(matlabdir)
            check_call([rsync, '-a', 'src/', matlabdir + '/xunit'])


@builder
def narrative():
    version = '0.1.2'
    srcdir = 'Narrative-' + version
    zipfile = srcdir + '.zip'

    with done_file(srcdir):
        if not os.path.isdir(srcdir):
            download_archive_from_sf('narrative/narrative',
                                     urllib.quote(srcdir.replace('-', ' ')),
                                     zipfile)
            unpack_zipfile(zipfile, srcdir)

        with workdir('/'.join([srcdir, srcdir, 'Narrative'])):
            check_call([
                xcodebuild,
                '-project', 'Narrative.xcodeproj',
                '-configuration', 'Release',
                '-xcconfig', os.path.join(xcconfigdir, 'macOS.xcconfig'),
                'clean',
                'build',
                'INSTALL_PATH=@loader_path/../Frameworks',
                'OTHER_CFLAGS=-fno-objc-arc -fno-objc-weak -fvisibility=default',
                ])

            check_call([
                rsync,
                '-a',
                'build/Release/Narrative.framework',
                frameworksdir
                ])


################################################################################
#
# Main function
#
################################################################################


def main():
    requested_builders = sys.argv[1:]
    builder_names = set(buildfunc.__name__ for buildfunc in all_builders)

    for name in requested_builders:
        if name not in builder_names:
            announce('ERROR: invalid builder: %r', name)
            sys.exit(1)

    make_directories(downloaddir, builddir)

    with workdir(builddir):
        for buildfunc in all_builders:
            if ((not requested_builders) or
                (buildfunc.__name__ in requested_builders)):
                try:
                    buildfunc()
                except DoneFileExists:
                    pass

    if not building_for_ios:
        # Install files
        check_call([
            rsync,
            '-a',
            '--exclude=*.la',
            '--exclude=pkgconfig',
            frameworksdir,
            matlabdir,
            includedir,
            libdir,
            os.environ['MW_DEVELOPER_DIR'],
            ])


if __name__ == '__main__':
    main()
