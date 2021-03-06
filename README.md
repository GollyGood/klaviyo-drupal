# Klaviyo API Integration

## Requirements

1. [Select (or other)](https://www.drupal.org/project/select_or_other)
2. [Entity API](https://www.drupal.org/project/entity)
3. [Libraries API](https://www.drupal.org/project/libraries)
4. [Klaviyo API PHP Library](https://github.com/GollyGood/klaviyo-api-php/archive/0.1.0.tar.gz). (See installation instructions below.)

## Installation

1. [Install the module per normal](https://www.drupal.org/documentation/install/modules-themes/modules-7).
2. Download [Klaviyo API PHP Library](https://github.com/GollyGood/klaviyo-api-php/archive/0.1.0.tar.gz).
3. Extract the archive to `/sites/all/libraries/klaviyo-api-php` such that `KlaviyoApi.php` is found at `/sites/all/libraries/klaviyo-api-php/src/KlaviyoApi.php`.
4. [Install Composer](https://getcomposer.org/doc/00-intro.md#installation-linux-unix-osx) and then run `composer install --no-dev` from within `sites/all/libraries/kalviyo-api-php`.

## Configuration

1. Navigate to `Administration >> Configuration >> Web services >> Klaviyo settings` (/admin/config/services/klaviyo) and set your API key.
2. Navigate to `Administration >> Configuration >> Account settings` (/admin/config/people/accounts) to enable Klaviyo integration for Drupal user accounts.

## Supporting Organizations

* [Illinois Legal Aid Online](http://www.illinoislegalaidonline.org)
* [GollyGood Software](https://www.gollygoodsoftware.com)
