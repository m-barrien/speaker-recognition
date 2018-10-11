from http.server import HTTPServer, BaseHTTPRequestHandler
import ssl
import GPIORelay

secret="abranlawea"
relay = GPIORelay(12)
class RelayHTTPRequestHandler(BaseHTTPRequestHandler):

    def do_GET(self):
        if secret in self.path:
        	self.send_response(200)
        	relay.openDoor()
        else:
        	self.send_error(404)
        self.send_header('Access-Control-Allow-Origin', '*')
        self.end_headers()
        self.wfile.write(b'Hello, world!')


httpd = HTTPServer(('', 8082), RelayHTTPRequestHandler)

#httpd.socket = ssl.wrap_socket (httpd.socket, keyfile="crt/key.pem", certfile='crt/cert.pem', server_side=True)
try:
	httpd.serve_forever()
except KeyboardInterrupt as e:
	exit(0);
finally:
	pass