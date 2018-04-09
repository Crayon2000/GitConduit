# GitConduit

GitConduit is a Windows application use to export git repositories from one Git web platform to another.

It supports the following services:
* Gogs
* GitBucket
* GitHub

## How To Use
* Make sure *git.exe* is in your path and credentials are saved
* Fill the **Source** section and click on the **Next** button
* Select the repositories you want to export and click on the **Next** button
* Fill the **Destination** section and click on the **Next** button
* Wait and look at the **Log** for more information
* Click on the **Close** button to exit the application

## Settings

### Gogs

#### API URL
The v1 API is used and the URL should look like: https://myserver.com/api/v1

#### Authorization Token

To get an authorization token:
1. Go to **Your Settings**
2. Select **Applications**
3. Click on **Generate New Token**
4. Choose a **Token Name**
5. Click on the **Generate Token** button

### GitBucket

#### API URL
The v3 API is used and the URL should look like: https://myserver.com/api/v3

#### Authorization Token

To get an authorization token:
1. Go to **Account settings**
2. Select **Applications**
3. Choose a **Token description**
4. Click on the **Generate token** button

### GitHub

#### API URL
The v3 API is used and the URL is https://api.github.com

#### Authorization Token

To get an authorization token:
1. Go to **Settings**
2. Select **Developer settings**
3. Select **Personal access tokens**
4. Click on **Generate new token**
5. Enter your password if required
6. Choose a **Token description** and select scopes
7. Click on the **Generate token** button
