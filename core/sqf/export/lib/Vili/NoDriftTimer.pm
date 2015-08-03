# @@@ START COPYRIGHT @@@
#
# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.
#
# @@@ END COPYRIGHT @@@

package Vili::NoDriftTimer;

=head1 NAME

Vili::NoDriftTimer - a module that contains package NoDriftTimer

=head1 SYNOPSIS

   use Vili::NoDriftTimer;

   my $timer = Vili::NoDriftTimer->new;

   while (1) {
      $timer->reset;

      ...

      $timer->sleep($nseconds);
   }

=head1 DESCRIPTION

A Perl class that provides a timer that has no drift.

=back

=head1 COPYRIGHT

(C) Copyright 2011-2014 Hewlett-Packard Development Company, L.P.

=cut

use strict;
use warnings;

sub new
{
   my $class = shift;
   my $interval = shift;

   my $self = {};

   $self->{default_interval} = defined($interval) ? $interval : 5.0;
    
   bless ($self, $class);

   $self->reset;
   return $self;
}

sub reset
{
   my $self = shift;
   $self->{last_time} = Time::HiRes::time;
}

sub sleep
{
   my $self = shift;
   my $interval = shift;

   $interval = $self->{default_interval} unless(defined($interval));

   my $curr_time = Time::HiRes::time;

   my $adjusted_interval = $interval - ($curr_time - $self->{last_time});

   $self->{last_time} += $interval;

   # make sleep uninterruptable by signal handler
   while ($adjusted_interval > 0) {
       my $slept_seconds = Time::HiRes::sleep($adjusted_interval);
       $adjusted_interval -= $slept_seconds;
   }
}

1;
