NGSI Event Broker
_________________


Nagios event broker (`NEB`_) module to forward plugin data to
`NGSI Adapter <../ngsi_adapter/README.rst>`__. Currently, the
broker is particularized for `XIFI`_ monitoring:

-  *ngsi\_event\_broker\_xifi* to process plugin executions for XIFI


Installation
============

The module is an architecture-dependent compiled shared object distributed as
a single library bundled in a Debian (.deb) or RedHat (.rpm) package. Assuming
FIWARE package repositories are configured, just use the proper tool (such as
``apt-get`` or ``rpm``) to install ``fiware-monitoring-ngsi-event-broker``
package. These distributions are currently supported:

-  Ubuntu 12.04 LTS

As an alternative, module can be compiled from sources, either downloaded from
sources repository or as `source code tarball <../README.rst#Releases>`__.
First option requires *autotools* and *libtool* to be installed, in order
to generate configuration script

.. code::

   $ mkdir m4
   $ autoreconf --install

Once ``configure`` script is generated (or downloaded as part of source
tarball), follow these steps:

.. code::

   $ ./configure
   $ make
   $ make check
   $ sudo make install

Default installation directory is ``/opt/fiware/ngsi-event-broker/lib``. In
case of manual installation, target directory can be changed by running
``./configure --libdir=target_libdir`` (may require sudoer privileges).


Usage
=====

Nagios should be instructed to load this module on startup. First, stop Nagios
service and then edit configuration file at ``/etc/nagios/nagios.cfg`` to add
the new broker module with its arguments: the id of the `region`__ that current
infrastructure belongs to, and the endpoint of NGSI Adapter component to
request:

.. code::

   event_broker_options=-1
   broker_module=/path/ngsi_event_broker_xifi.so -r region -u http://host:port

The module will use such information given as arguments together with data taken
from the `Nagios service definition`_ to issue a request to NGSI Adapter. In
many cases, service definitions need no modifications and the broker just works
transparently once Nagios is restarted. But there are some scenarios requiring
slight changes in those service definitions (see below).

Once main configuration file and service definitions have been reviewed, then
start Nagios service. Check log files for module initialization (may fail for
missing arguments, for example). Also check that requests are sent to Adapter
server in response to plugin executions.

__ `OpenStack region`_


Service definitions
~~~~~~~~~~~~~~~~~~~

Assuming this Nagios host definition:

.. code::

   define host{
       use                     linux-server
       host_name               myhostname
       alias                   linux_server
       address                 192.168.0.2
       }

then a typical Nagios service definition would look like this:

.. code::

   define service{
       use                     generic-service
       host_name               myhostname
       service_description     my service description
       check_command           check_name!arguments
       ...
       }

Depending on the entities being monitored (thus depending on the kind of plugins
used), some of these data items are taken and some additional may be required.
Requests to NGSI Adapter issued by this broker will all follow the pattern
``http://{host}:{port}/{check_name}?id={region}:{uniqueid}&type={type}``, where:

-  ``http://{host}:{port}`` is the endpoint taken from broker arguments
-  ``{check_name}`` is taken from Nagios command specified at service definition
-  ``{region}`` is taken from broker arguments
-  ``{uniqueid}`` is taken from service definition, depending on the command
   plugin
-  ``{type}`` is also taken from service definition, also depending on the
   command

For *SNMP monitoring* a Nagios command named ``check_snmp`` should be used.
Entity type ``interface`` is assumed by default and ``{uniqueid}`` consists
of the address and port number given as command arguments (see ``check_snmp``
manpage). Entity id in requests would be ``{region}:{ifaddr}/{ifport}``

For *host service monitoring* there are no restrictions on the command names
and the plugins to be used. The ``{uniqueid}`` consists of the hostname and
description of the service, resulting an entity id
``{region}:{hostname}:{servicedesc}``. However, the exact entity type must be
explicitly given with a custom variable ``_entity_type`` at service definition
(or using templates, as follows):

.. code::

   define service{
       use                     generic-service
       name                    host-service
       _entity_type            host_service
       }

   define service{
       use                     host-service
       host_name               myhostname
       service_description     my service description
       check_command           check_name!arguments
       ...
       }

For *any other plugin executed locally* the entity id will include the local
address and a ``host`` entity type will be assumed, resulting a request like
``http://{host}:{port}/{check_name}?id={region}:{localaddr}&type=host``

For *any other plugin executed remotely via NRPE* the entity id will include
the remote address instead, a ``vm`` entity type will be assumed and the
``{check_name}`` will be taken from arguments of ``check_nrpe`` plugin.

Default entity types may be superseded in any case by including in the service
definition the aforementioned custom variable ``_entity_type``.


Changelog
=========

Version 1.4.1

-  Minor bugs resolved

Version 1.4.0

-  Included new log format (issue #25)

Version 1.3.1

-  Included Debian package generation
-  Fixed error in argument parser

Version 1.3.0

-  Included "host\_service" monitoring

Version 1.2.0

-  Unification into a single \_xifi broker

Version 1.1.0

-  Broker splitted into \_snmp and \_host
-  IP address as unique identifier (within region) for hosts and vms
-  Added region as argument
-  Added NRPE support

Version 1.0.1

-  Added regions support (value retrieved from a metadata key named
   "region")

Version 1.0.0

-  Initial release of the module


License
=======

\(c) 2013-2015 Telefónica I+D, Apache License 2.0


.. REFERENCES

.. _XIFI: https://www.fi-xifi.eu/home.html
.. _NEB: http://nagios.sourceforge.net/download/contrib/documentation/misc/NEB%202x%20Module%20API.pdf
.. _Nagios service definition: http://nagios.sourceforge.net/docs/3_0/objectdefinitions.html#service
.. _OpenStack region: http://docs.openstack.org/glossary/content/glossary.html#region
