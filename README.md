This is a fork of the [Drupal 7 CAS module](https://drupal.org/project/cas) that supports authenticaton against
multiple CAS servers. There is an issue to potentially get this code ported to the CAS module as version 2.0:
https://www.drupal.org/node/2269875

## Overview of Features
* Define any number of CAS server configurations that users on your site can use to authenticate against. For each server configuration, you can define the basic details (hostname, port, CAS server verison, etc), automatic user registration, automatic role assignment, prevent CAS users from changing their email/password, control whether or not a link to login to this CAS server is present on the user/login form (and provide a link label and description), and single sign out support.
* The user/login page by default is untouched. When you add a new CAS server configuration, you can opt to include a link above the normal Drupal login form to authenticate using the CAS server. You can do this for each CAS server you define. Themers can override the user login page as needed to style it better.
* Server configurations are exportable thanks to ctools. This means you can featurize the configuration of the entire module easily.
* Specify a default CAS server to authenticate against when users visit /cas. Otherwise, each CAS server has its own special authentication path, like /cas/server1, /cas/server2
* Each Drupal user can be associated with usernames existing in one or more CAS servers. This you can have a local Drupal account "foo" that can authenticate with CAS server 1 user "bar" and CAS server 2 user "baz". There's an interface for each user to manage the CAS user associations they have.

## Important Considerations

1. The [CAS Attributes](https://www.drupal.org/project/cas_attributes) and [CAS Roles](https://www.drupal.org/project/cas_roles) modules
may continue to work but they provide no way to offer different configuration depending on the CAS server
a user authenticated with. The hooks that these modules implement are passed information about the CAS server,
they just need to be updated to make use of it.
2. There are no automated tests.
3. Support for "forced auth" and "gateway login" have been removed for simplicity. But could work, but
they were not required for the specific use case this fork was created for.
4. The login block which simply renders a "Login" button to CAS was not added, since the same functionality
can easily be added with a normal Drupal block.
5. The CAS Server sub-module was removed.
