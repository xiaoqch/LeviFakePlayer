
# for lse-quickjs
cd .\lse-api
npm i -y
npm run test
cd ..

cp -r -Force lse-api\dist\lfp4lse*-test .\bin\

cp .\lse-api\manifest.js.json .\bin\lfp4lsejs-test\manifest.json

