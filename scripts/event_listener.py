from argparse import ArgumentParser
from http.server import BaseHTTPRequestHandler
from http.server import HTTPServer


class RequestHandler(BaseHTTPRequestHandler):
    def _set_response(self):
        self.send_response(200)
        self.send_header('Content-type', 'text/html')
        self.end_headers()

    def do_GET(self):
        print("GET request,\nPath: %s\nHeaders:\n%s\n" % (
            str(self.path), str(self.headers)))
        self._set_response()
        self.wfile.write(
            "GET request for {}\n".format(self.path).encode('utf-8'))

    def do_POST(self):
        content_length = int(
            self.headers['Content-Length'])
        post_data = self.rfile.read(content_length)
        print("POST request,\nPath: %s\nHeaders:\n%s\n\nBody:\n%s\n" %
              (str(self.path), str(self.headers), post_data.decode('utf-8')))

        self._set_response()

    do_PUT = do_POST
    do_DELETE = do_GET


def main(args):
    port = args.port
    ip = args.ip
    print('Listening on %s:%s' % (ip, port))
    server = HTTPServer((ip, port), RequestHandler)
    server.serve_forever()


if __name__ == "__main__":
    parser = ArgumentParser(
        description="Server that returns nothing for any http request made to "
                    "it; good for testing Redfish Eventing")
    parser.add_argument('-i', '--ip', default='localhost',
                        help='host to bind to')
    parser.add_argument('-p', '--port', default=3246, type=int,
                        help='port to listen on')
    args = parser.parse_args()

    main(args)
