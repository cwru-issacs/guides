#!/usr/bin/env python3
# Copyright 2017 Google Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""A demo of the Google CloudSpeech recognizer."""
import argparse
from functools import partial
import locale
import logging
import subprocess
from time import sleep
import urllib.request

import Adafruit_DHT
import aiy.voice.tts
from aiy.board import Board, Led
from aiy.cloudspeech import CloudSpeechClient


def get_hints(language_code):
    if language_code.startswith('en_'):
        return ('change banner to think[box]',
                'change banner to ISSACS',
                'shutdown',
                'check the temperature')
    return None

def locale_language():
    language, _ = locale.getdefaultlocale()
    return language

def main():
    logging.basicConfig(level=logging.DEBUG)

    parser = argparse.ArgumentParser(description='Assistant service example.')
    parser.add_argument('--language', default=locale_language())
    args = parser.parse_args()

    logging.info('Initializing for language %s...', args.language)
    hints = ("ok google",)
    client = CloudSpeechClient()
    with Board() as board:
        board.button.when_pressed = partial(wait_for_speech, args.language, client, board)
        while True:
            board.led.state = Led.OFF
            if hints:
                logging.info('Say something, e.g. %s.' % ', '.join(hints))
            else:
                logging.info('Say something.')
            text = client.recognize(language_code=args.language,
                                    hint_phrases=hints)
            if text is None:
                logging.info('You said nothing.')
                continue

            logging.info('You said: "%s"' % text)
            text = text.lower()
            if 'ok google' == text:
                wait_for_speech(args.language, client, board)
            elif 'ok google' in text:
                wait_for_speech(args.language, client, board, text)

def wait_for_speech(language_code, client, board, text=None):
    board.led.state = Led.ON
    if text is None:
        hints = get_hints(language_code)
        if hints:
            logging.info('Say something, e.g. %s.' % ', '.join(hints))
        else:
            logging.info('Say something.')
        text = client.recognize(language_code=language_code,
                                hint_phrases=hints)
        if text is None:
            logging.info('You said nothing.')
            return

    board.led.state = Led.BLINK

    #logging.info('You said: "%s"' % text)
    text = text.lower()
    if 'change banner to' in text or 'set banner to' in text or 'turn banner to' in text:
        if 'think box' in text or 'thinkbox' in text or 'pink box' in text:
            urllib.request.urlopen("http://172.19.34.203/banner0")
            logging.info('Changed to think[box]')
            aiy.voice.tts.say('Ok, the banner now says think box')
        elif 'isaacs' in text or 'issacs' in text or 'isaac' in text:
            urllib.request.urlopen("http://172.19.34.203/banner1")
            logging.info('Changed to ISSACS')
            aiy.voice.tts.say("Ok, the banner now says Isaac's")
        elif 'disco' in text:
            urllib.request.urlopen("http://172.19.34.203/disco")
            logging.info('Changed to disco')
            aiy.voice.tts.say("Ok, the banner is now set to disco")
    elif 'weather' in text:
        humidity, temperature = Adafruit_DHT.read_retry(11, 24)
        logging.info('Temperature: ' + str(temperature * 9 / 5 + 32) + '°F, Humidity: ' + str(humidity) + '%')
        aiy.voice.tts.say('The temperature is ' + str(temperature * 9 / 5 + 32).rstrip('0').rstrip('.') + '°F')
        aiy.voice.tts.say('The humidity is at ' + str(humidity).rstrip('0').rstrip('.') + '%')
    elif 'temperature' in text:
        humidity, temperature = Adafruit_DHT.read_retry(11, 24)
        logging.info('Temperature: ' + str(temperature * 9 / 5 + 32) + '°F')
        aiy.voice.tts.say('The temperature is ' + str(temperature * 9 / 5 + 32).rstrip('0').rstrip('.') + '°F')
    elif 'humidity' in text or 'humid' in text:
        humidity, temperature = Adafruit_DHT.read_retry(11, 24)
        logging.info('Humidity: ' + str(humidity) + '%')
        aiy.voice.tts.say('The humidity is at ' + str(humidity).rstrip('0').rstrip('.') + '%')
    elif 'shut down' in text or 'shutdown' in text or 'turn off' in text:
        logging.info('Shutting down')
        aiy.voice.tts.say('Shutting down')
        subprocess.call(["sudo", "shutdown", "-h", "now"])
    board.led.state = Led.OFF
    

if __name__ == '__main__':
    main()
