# gets the list of torrents that ought to be in progress, gets the list of torrents that are in progress,
# and resolves any discrepancies.
# transmission-daemon is expected to have rpc available at http://localhost:9091/transmission/rpc
import requests
from config import config
import os
import sys
import json
import base64

assert 'all_torrents_url' in config
assert 'torrent_url' in config

if 'server_ca' in config:
	os.environ['REQUESTS_CA_BUNDLE'] = config['server_ca']
cert = config['client_cert'] if 'client_cert' in config else None

# get the list of current (active) torrents.
r = requests.get(config['all_torrents_url'], cert=cert)

active_torrents = set(r.text.splitlines())
active_torrents.discard('') # ensure any empty lines get thrown out

r2 = requests.post('http://localhost:9091/transmission/rpc')
headers = {'X-Transmission-Session-Id': r2.headers['X-Transmission-Session-Id']}
r2 = requests.post('http://localhost:9091/transmission/rpc', headers=headers,
	json={'method': 'torrent-get', 'arguments': {'fields': ['hashString']}})

current_torrents = set([t['hashString'] for t in r2.json()['arguments']['torrents']])

to_remove = current_torrents - active_torrents
to_add = active_torrents - current_torrents

if len(active_torrents) < 100:
	# fewer than 100 torrents? not on MY server.
	print('Too few torrents from server.  Not making changes', file=sys.stderr)
	sys.exit(1)

print(json.dumps({'to_add': list(to_add), 'to_remove': list(to_remove)}, indent=4))

if current_torrents and len(to_remove)/len(current_torrents) > 0.1:
	print('Tried to remove >10% of current torrents.  Bailing', file=sys.stderr)
	sys.exit(1)

transmission_message = {
	'method': 'torrent-remove',
	'arguments': {
		'ids': list(to_remove),
		'delete-local-data': True
	}
}

r2 = requests.post('http://localhost:9091/transmission/rpc', headers=headers,
	json=transmission_message)

for hashString in to_add:
	r = requests.get(config['torrent_url'].format(hash=hashString), cert=cert)
	data = r.content
	if r.status_code != 200 or not data:
		print('Invalid response for hash %s' % hashString, file=sys.stderr)
		continue
	message = {
		'method': 'torrent-add',
		'arguments': {
			'metainfo': base64.b64encode(data).decode('ascii')
		}
	}
	r2 = requests.post('http://localhost:9091/transmission/rpc', headers=headers, json=message)
