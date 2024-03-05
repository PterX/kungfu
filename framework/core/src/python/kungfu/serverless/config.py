from kungfu.yijinjing.utils import get_default_home_dir
from kungfu.serverless.params import alpha, dev, prod
import os


AUTHING_APP_CONFIG = {
    "dev": {
        "appId": "656839e2d2d74b312123455c",
        "appSecret": "eed96a7726e3e40ce1648f514f05fcc1",
        "appHost": "https://serverless-kungfu-dev.authing.cn",
    },
    "alpha": {
        "appId": "656ff475456425a56da6c42c",
        "appSecret": "e2e791fa27594b67eb0b48b15881af7b",
        "appHost": "https://serverless-kungfu-alpha.authing.cn",
    },
    "prod": {
        "appId": "65c09924846922437110d676",
        "appSecret": "8730916236a807d1f1406a2d5baa5b3c",
        "appHost": "https://serverless-kungfu.authing.cn",
    },
}


TOKEN_FILE = os.path.join(
    os.path.dirname(get_default_home_dir()), "config", "authingCredentials.json"
)

APP_PARAMS = {
    "alpha": alpha.PARAMS,
    "dev": dev.PARAMS,
    "prod": prod.PARAMS,
}
