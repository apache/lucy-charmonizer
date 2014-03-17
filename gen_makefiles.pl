#!/usr/bin/perl

# Licensed to the Apache Software Foundation (ASF) under one or more
# contributor license agreements.  See the NOTICE file distributed with
# this work for additional information regarding copyright ownership.
# The ASF licenses this file to You under the Apache License, Version 2.0
# (the "License"); you may not use this file except in compliance with
# the License.  You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

use strict;
use warnings;

package Charmonizer::Build::Makefile;
use File::Find qw();
use FindBin;
use Carp qw( confess );
use Cwd qw( getcwd );

sub new {
    my ( $class, %args ) = @_;

    # Validate args, create object.
    for (qw( dir filename obj_ext exe_ext cc )) {
        defined $args{$_} or confess("Missing required param '$_'");
    }
    my $dir  = $args{dir};
    my $self = bless {
        dir          => $dir,
        filename     => $args{filename},
        obj_ext      => $args{obj_ext},
        exe_ext      => $args{exe_ext},
        cc           => $args{cc},
        extra_cflags => $args{extra_cflags} || '',
        extra_clean  => $args{extra_clean} || '',
    }, $class;

    # Gather source paths, normalized for the target OS.
    my $orig_dir = getcwd();
    chdir($dir);
    -d 'src' or confess("Can't find 'src' directory within '$dir'");
    my ( @h_files, @c_files, @c_tests );
    push @c_files, "charmonize.c";
    File::Find::find(
        {   wanted => sub {
                if (/\.c$/) {
                    if (/^Test/) {
                        push @c_tests, $File::Find::name;
                    }
                    else {
                        push @c_files, $File::Find::name;
                    }
                }
                elsif (/\.h$/) {
                    push @h_files, $File::Find::name;
                }
            },
        },
        'src',
    );
    chdir($orig_dir);
    $self->{c_files} = [ sort map { $self->pathify($_) } @c_files ];
    $self->{h_files} = [ sort map { $self->pathify($_) } @h_files ];
    $self->{c_tests} = [ sort map { $self->pathify($_) } @c_tests ];
    $self->{c_test_cases}
        = [ grep { $_ !~ /Test\.c/ } @{ $self->{c_tests} } ];

    return $self;
}

sub pathify { confess "abstract method" }

sub unixify {
    my ( $self, $path ) = @_;
    $path =~ tr{\\}{/};
    return $path;
}

sub winnify {
    my ( $self, $path ) = @_;
    $path =~ tr{/}{\\};
    return $path;
}

sub objectify {
    my ( $self, $c_file ) = @_;
    $c_file =~ s/\.c$/$self->{obj_ext}/ or die "No match: $c_file";
    return $c_file;
}

sub execify {
    my ( $self, $file ) = @_;
    $file =~ s/.*?(\w+)\.c$/$1$self->{exe_ext}/ or die "No match: $file";
    return $file;
}

sub build_link_command {
    my ( $self, %args ) = @_;
    my $objects = join( " ", @{ $args{objects} } );
    return "\$(CC) \$(CFLAGS) $objects -o $args{target}";
}

sub c2o_rule {
    qq|.c.o:\n\t\$(CC) \$(CFLAGS) -c \$*.c -o \$@|;
}

sub test_block {
    my ( $self, $c_test_case ) = @_;
    my $exe = $self->execify($c_test_case);
    my $obj = $self->objectify($c_test_case);
    my $test_obj
        = $self->pathify( $self->objectify("src/Charmonizer/Test.c") );
    my $link_command = $self->build_link_command(
        objects => [ $obj, $test_obj ],
        target  => '$@',
    );
    return qq|$exe: $test_obj $obj\n\t$link_command|;
}

sub clean_rule { confess "abstract method" }

sub clean_rule_posix {
    qq|clean:\n\trm -f \$(CLEANABLE)|;
}

sub clean_rule_win {
    qq|clean:\n\tCMD /c FOR %i IN (\$(CLEANABLE)) DO IF EXIST %i DEL /F %i|;
}

sub meld_rule { confess "abstract method" }

sub meld_rule_posix {
    qq|meld:\n\t\$(PERL) buildbin/meld.pl --probes=\$(PROBES) |
        . qq|--files=\$(FILES) --out=\$(OUT)|;
}

sub meld_rule_win {
    qq|meld:\n\t\$(PERL) buildbin\\meld.pl --probes=\$(PROBES) |
        . qq|--files=\$(FILES) --out=\$(OUT)|;
}

sub charmony_h_rule { confess "abstract method" }

sub charmony_h_rule_posix {
    return <<"EOF";
\$(CHARMONY_H): \$(PROGNAME)
\t./\$(PROGNAME) --cc=\$(CC) --enable-c
EOF
}

sub charmony_h_rule_win {
    return <<"EOF";
\$(CHARMONY_H): \$(PROGNAME)
\t\$(PROGNAME) --cc=\$(CC) --enable-c
EOF
}

sub test_rule { confess "abstract method" }

sub test_rule_posix {
    return <<"EOF";
test: tests
\tprove ./Test*
EOF
}

sub test_rule_win {
    return <<"EOF";
test: tests
\tprove Test*
EOF
}

sub gen_makefile {
    my ( $self, %args ) = @_;
    my ( $h_files, $c_files, $c_tests, $c_test_cases )
        = @$self{qw( h_files c_files c_tests c_test_cases )};

    # Derive chunks of Makefile content.
    my $progname              = $self->execify('charmonize.c');
    my $c2o_rule              = $self->c2o_rule;
    my $meld_rule             = $self->meld_rule;
    my $charmony_h_rule       = $self->charmony_h_rule;
    my $test_rule             = $self->test_rule;
    my $progname_link_command = $self->build_link_command(
        objects => ['$(OBJS)'],
        target  => '$(PROGNAME)',
    );
    my $clean_rule  = $self->clean_rule;
    my $objs        = join " ", map { $self->objectify($_) } @$c_files;
    my $test_objs   = join " ", map { $self->objectify($_) } @$c_tests;
    my $test_blocks = join "\n\n",
        map { $self->test_block($_) } @$c_test_cases;
    my $test_execs = join " ", map { $self->execify($_) } @$c_test_cases;
    my $headers = join " ", @$h_files;

    # Write out Makefile content.
    open my $fh, ">", $self->{filename}
        or die "open '$self->{filename}' failed: $!\n";
    my $content = <<EOT;
# GENERATED BY $FindBin::Script: do not hand-edit!!!

# Licensed to the Apache Software Foundation (ASF) under one or more
# contributor license agreements.  See the NOTICE file distributed with
# this work for additional information regarding copyright ownership.
# The ASF licenses this file to You under the Apache License, Version 2.0
# (the "License"); you may not use this file except in compliance with
# the License.  You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

CC= $self->{cc}
DEFS=
CFLAGS= -I. -Isrc \$(DEFS) $self->{extra_cflags}
PROGNAME= $progname
CHARMONY_H= charmony.h
PROBES=
FILES=
OUT=
PERL=/usr/bin/perl

TESTS= $test_execs

OBJS= $objs

TEST_OBJS= $test_objs

HEADERS= $headers

CLEANABLE= \$(OBJS) \$(PROGNAME) \$(CHARMONY_H) \$(TEST_OBJS) \$(TESTS) $self->{extra_clean}

$c2o_rule

all: \$(PROGNAME)

$meld_rule

$charmony_h_rule

\$(PROGNAME): \$(OBJS)
\t$progname_link_command

\$(OBJS) \$(TEST_OBJS): \$(HEADERS)

\$(TEST_OBJS): \$(CHARMONY_H)

tests: \$(TESTS)

$test_blocks

$test_rule

$clean_rule

EOT
    print $fh $content;
}

package Charmonizer::Build::Makefile::Posix;
BEGIN { our @ISA = qw( Charmonizer::Build::Makefile ) }

sub new {
    my $class = shift;
    return $class->SUPER::new(
        filename => 'Makefile',
        obj_ext  => '.o',
        exe_ext  => '',
        cc       => 'cc',
        @_
    );
}

sub clean_rule      { shift->clean_rule_posix }
sub meld_rule       { shift->meld_rule_posix }
sub charmony_h_rule { shift->charmony_h_rule_posix }
sub test_rule       { shift->test_rule_posix }
sub pathify         { shift->unixify(@_) }

package Charmonizer::Build::Makefile::MSVC;
BEGIN { our @ISA = qw( Charmonizer::Build::Makefile ) }

sub new {
    my $class = shift;
    my $flags
        = '-nologo -D_CRT_SECURE_NO_WARNINGS ' . '-D_SCL_SECURE_NO_WARNINGS';
    return $class->SUPER::new(
        filename     => 'Makefile.MSVC',
        obj_ext      => '.obj',
        exe_ext      => '.exe',
        cc           => 'cl',
        extra_cflags => $flags,
        extra_clean  => '*.pdb',
        @_
    );
}

sub c2o_rule {
    qq|.c.obj:\n\t\$(CC) \$(CFLAGS) -c \$< -Fo\$@|;
}

sub build_link_command {
    my ( $self, %args ) = @_;
    my $objects = join( " ", @{ $args{objects} } );
    return "link -nologo $objects /OUT:$args{target}";
}

sub pathify         { shift->winnify(@_) }
sub clean_rule      { shift->clean_rule_win }
sub meld_rule       { shift->meld_rule_win }
sub charmony_h_rule { shift->charmony_h_rule_win }
sub test_rule       { shift->test_rule_win }

package Charmonizer::Build::Makefile::MinGW;
BEGIN { our @ISA = qw( Charmonizer::Build::Makefile ) }

sub new {
    my $class = shift;
    return $class->SUPER::new(
        filename => 'Makefile.MinGW',
        obj_ext  => '.o',
        exe_ext  => '.exe',
        cc       => 'gcc',
        @_
    );
}

sub pathify         { shift->winnify(@_) }
sub clean_rule      { shift->clean_rule_win }
sub meld_rule       { shift->meld_rule_win }
sub charmony_h_rule { shift->charmony_h_rule_win }
sub test_rule       { shift->test_rule_win }

### actual script follows
package main;

my $makefile_posix = Charmonizer::Build::Makefile::Posix->new( dir => '.' );
my $makefile_msvc = Charmonizer::Build::Makefile::MSVC->new( dir => '.' );
my $makefile_mingw = Charmonizer::Build::Makefile::MinGW->new( dir => '.' );
$makefile_posix->gen_makefile;
$makefile_msvc->gen_makefile;
$makefile_mingw->gen_makefile;

__END__

=head1 NAME

gen_charmonizer_makefiles.pl

=head1 SYNOPSIS

    gen_charmonizer_makefiles.pl - keeps the Makefiles in sync with the live tree.

=head1 DESCRIPTION

Be sure to run this code from the charmonizer subdirectory (where the
existing Makefiles live).

