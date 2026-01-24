#!/usr/bin/env python3
from http.server import SimpleHTTPRequestHandler, HTTPServer
import http.server


class CORSHandler(SimpleHTTPRequestHandler):
    def end_headers(self):
        # 添加 CORS 头
        self.send_header("Access-Control-Allow-Origin", "*")
        # Python 3 中推荐使用 super() 来调用父类方法
        super().end_headers()


if __name__ == "__main__":
    # http.server.test 是 Python 内置的简易启动函数
    # 默认端口为 8000，也可以通过 port 参数指定
    print("Serving HTTP on 0.0.0.0 port 8000 ...")
    http.server.test(HandlerClass=CORSHandler, ServerClass=HTTPServer)
