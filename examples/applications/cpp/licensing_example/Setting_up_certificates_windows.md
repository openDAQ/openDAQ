In order to sign a binary file such as a module DLL in Windows, the user must create a certificate for it.

To learn how to work with certificates properly, go to "[Working with certificates](https://learn.microsoft.com/en-us/dotnet/framework/wcf/feature-details/working-with-certificates)". Here are just a quick set of instructions for this particular case.

Depending on the level of security one wishes, one can setup a single self-signed certificate or create/make it part of a certificate trust chain to a trusted root certificate installed on windows (to view the root certificates, search "Manage Computer Certificates"/"Manage User Certificates" in the windows search bar to open up the **Microsoft Management Console - MMC**). The `WinVerifyTrust` functionfrom the **WinCrypt** API, found in the example `ModuleAuthenticatorImpl`, will return an appropriate return value depending on whether the signature of the DLL is trusted by Windows (i.e. is part of a valid certificate chain), whether the root is untrusted, whether the certificate is expired...

In order to setup a simple certificate chain for development, we need two certificates. The root certificate and the signature certificate. The root certificate will be installed in the "Trusted Root Certificate Authorities" (with the MMC). The signature certificate will be signed by the root certificate, and the DLL will be signed by the signature certificate, creating a trust chain. 

The following commands are taken and modified from [How to: Create Temporary Certificates for Use During Development](https://learn.microsoft.com/en-us/dotnet/framework/wcf/feature-details/how-to-create-temporary-certificates-for-use-during-development). The complete script (apart from signing) can be run from "setup_cert_chain.ps1".

### Setting up the root certificate

To setup the root certificate, we create it in PowerShell with: 

```powershell
$rootCert = New-SelfSignedCertificate -Type CodeSigningCert -CertStoreLocation Cert:\CurrentUser\My -DnsName "RootCA" -TextExtension @("2.5.29.19={text}CA=true") -KeyUsage CertSign,CrlSign,DigitalSignature
```

The important addition here is `-Type CodeSigningCert`. Without it the certificate 'can' sign a DLL, but Windows will not trust the signature.

Next, we export it to .pfx file:

```powershell
[System.Security.SecureString]$rootCertPassword = ConvertTo-SecureString -String "password" -Force -AsPlainText
[String]$rootCertPath = Join-Path -Path 'cert:\CurrentUser\My\' -ChildPath "$($rootCert.Thumbprint)"
Export-PfxCertificate -Cert $rootCertPath -FilePath 'RootCA.pfx' -Password $rootCertPassword
```

### Importing it

Open up the MMC, right click on the "**Trusted Root Certification Authorities**" folder and select **All Tasks -> Import**. Follow the installation wizard and select the .pfx file to import.

NOTE: When done developing, it's better to clean up any test certificate from this folder.

### Setting up the signing certificate

Setting up the code signing certificate is similar to setting up the root:

```powershell
$testCert = New-SelfSignedCertificate -Type CodeSigningCert -Subject "OpenDAQ Test Certificate" -CertStoreLocation Cert:\CurrentUser\My -DnsName "SignedByRootCA" -KeyExportPolicy Exportable -KeyLength 2048 -KeyUsage DigitalSignature,KeyEncipherment -Signer $rootCert
```

The key difference here is setting the `-Signer` to our root certificate.

We then export it in the same way. Note that we need the root password (`-Password $rootCertPassword`) to export the signed certificate:

```powershell
[String]$testCertPath = Join-Path -Path 'cert:\CurrentUser\My\' -ChildPath "$($testCert.Thumbprint)"
Export-PfxCertificate -Cert $testCertPath -FilePath testcert.pfx -Password $rootCertPassword
```

### Signing the DLL

Given the `licensing_module-64-3.module.dll` (or `licensing_module-64-3-debug.module.dll`), we sign it using the **Visual Studio developer prompt**, which gives us access to the `signtool.exe` (you might need to install it, a quick google search will help you out):

```
signtool.exe sign /v /n "OpenDAQ Test Certificate" /f "testcert.pfx" /p "password" /fd SHA256 /t http://timestamp.digicert.com "path/to/licensing_module-64-3.module.dll"
```

If you check the *Properties* of the DLL file and go to *Digital Signatures*, it should contain the signature. You can also verify it has a valid signature with the following command:

```
signtool.exe verify /v /pa "path/to/licensing_module-64-3.module.dll"
```

Finally, make sure to retrieve the thumbprint of your signature, as the example uses it as a second security step (in `main.cpp L134`):

```cpp
    // Add your signature hash here
    const StringPtr key = StringPtr("my_cert_thumbprint");
```

You can write it to a file, then discard it:

```PowerShell
New-Item .\signing_cert_thumbprint.txt -type file
$testCert.Thumbprint | Out-File .\signing_cert_thumbprint.txt
```

Now, the example `ModuleAuthenticator` should be able to authenticate the DLL on loading.
