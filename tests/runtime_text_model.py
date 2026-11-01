#!/usr/bin/env python3
"""Synthetic positioned-text runtime checks, no commercial resources."""
import argparse
import hashlib
import os
from pathlib import Path
import subprocess
import struct
import tempfile
import unittest
from runtime_fixtures import Script,archive,read_png,png
EXE=None
class TextRuntime(unittest.TestCase):
 def setUp(self):
  self.tmp=tempfile.TemporaryDirectory(prefix='yukari-positioned-text-');self.root=Path(self.tmp.name);self.game=self.root/'game';self.game.mkdir();self.state=self.root/'state'
 def tearDown(self):self.tmp.cleanup()
 def run_game(self,s,actions):
  package=self.game/'data01.ykc';package.write_bytes(archive([('start.yks',s.finish()),('base.png',png())]));before=hashlib.sha256(package.read_bytes()).digest()
  commands=self.root/'actions';commands.write_text('\n'.join(actions)+'\n')
  env=dict(os.environ,SDL_VIDEODRIVER='dummy',SDL_AUDIODRIVER='dummy',SDL_RENDER_DRIVER='software')
  p=subprocess.run([str(EXE),'play',str(self.game),'--fast','--mute','--state-dir',str(self.state),'--actions',str(commands),'--trace','--max-ms','10000'],env=env,capture_output=True,text=True,timeout=20)
  self.assertEqual(p.returncode,0,(p.stdout+p.stderr)[-6000:]);self.assertEqual(before,hashlib.sha256(package.read_bytes()).digest());return p.stdout+p.stderr
 def base(self,right=300,bottom=220):
  s=Script();s.call('GraphicLoad',0,'base.png',0,0);s.call('GraphicLoad',1,'base.png',0,0);s.call('DefaultFontSet',0,'test',20,400,20,20);s.call('DefaultFontSet',1,'ruby',8,400,8,8);s.call('TextWindowSet',0,100,100,right,bottom);s.call('FontSet',0,0);return s
 def test_overflow_save_restores_pending_host(self):
  s=self.base(140,140);s.call('StrOutNW',0,'ABCD');s.assign('GlobalFlag',9000,1);s.call('KeyWait');s.call('ScriptExit')
  first=self.root/'first.png';reloaded=self.root/'reload.png';last=self.root/'last.png'
  log=self.run_game(s,[f'capture {first}','save 7','advance','load 7',f'capture {reloaded}','advance',f'capture {last}','advance'])
  self.assertEqual(first.read_bytes(),reloaded.read_bytes(),'restoring pending output changed first page');self.assertNotEqual(first.read_bytes(),last.read_bytes(),'next page did not emit remaining glyphs');self.assertEqual(log.count('  StrOutNW'),1,'restored output host was replayed');self.assertIn('F 9000 1',(self.state/'yukari-system.dat').read_text())
  raw=(self.state/'savedata.007.yukari').read_bytes();self.assertIn(b'YUKARI_RUNTIME_9',raw)
 def test_style_ruby_name_and_native_positions(self):
  s=self.base();s.call('TextColor',0,0x000000ff);s.call('StrOutNW',0,'AA');s.call('TextColor',0,0x0000ff00);s.call('StrOutNW',0,'BB');s.call('RubyFontSet',1);s.call('StrOutNW',0,'@r(1,abc)');s.call('NameWindowSet',1,300,100,500,180);s.call('FontSet',1,0);s.call('NameOut','NAME');s.call('KeyWait');s.call('ScriptExit')
  first=self.root/'native-before.png';loaded=self.root/'native-after.png';log=self.run_game(s,[f'capture {first}','save 4','load-original 4',f'capture {loaded}','advance'])
  w,h,p=read_png(first);self.assertTrue(any(r>100 and r>g*2 for y in range(100,125) for x in range(100,120) for r,g,b in [p(x,y)]),'earlier red style was lost');self.assertTrue(any(g>100 and g>r*2 for y in range(100,125) for x in range(120,150) for r,g,b in [p(x,y)]),'later green style absent')
  self.assertEqual(first.read_bytes(),loaded.read_bytes(),'native import reflowed glyph coordinates/font/color');self.assertIn('LOAD_ORIGINAL_READY',log)
 def test_hidden_refresh_redraws_before_show_and_reload(self):
  s=self.base();s.call('GraphicHide',0);s.call('StrOutNW',0,'HIDDEN TEXT');s.call('DefaultFontEffect',3);s.call('GraphicRefresh');s.call('KeyWait');s.call('GraphicShow',0);s.call('KeyWait');s.call('GraphicRefresh');s.call('KeyWait');s.call('ScriptExit')
  hidden=self.root/'deferred-hidden.png';visible=self.root/'deferred-visible.png';loaded=self.root/'deferred-loaded.png';refreshed=self.root/'deferred-refresh.png'
  self.run_game(s,[f'capture {hidden}','advance',f'capture {visible}','save 9','load 9',f'capture {loaded}','advance',f'capture {refreshed}','advance'])
  self.assertNotEqual(hidden.read_bytes(),visible.read_bytes(),'hidden text became visible before its parent')
  self.assertEqual(visible.read_bytes(),loaded.read_bytes(),'deferred text changed after save reload')
  self.assertEqual(visible.read_bytes(),refreshed.read_bytes(),'show did not regenerate the latest hidden text styles')
 def test_hidden_window_restore_and_history_dedup(self):
  s=self.base();s.call('DefaultTextWindowGroup',0);s.call('StrOutNW',0,'ABC');s.call('HistoryRecord',0);s.call('HistoryRecord',0);s.call('TextWindowInvisible');s.call('KeyWait');s.call('ScriptExit')
  hidden=self.root/'hidden.png';visible=self.root/'visible.png';log=self.run_game(s,[f'capture {hidden}','advance',f'capture {visible}','save 4','advance'])
  self.assertNotEqual(hidden.read_bytes(),visible.read_bytes(),'TextWindowInvisible did not restore original visible layers')
  raw=(self.state/'savedata.004').read_bytes();at,size=struct.unpack_from('<II',raw,0x28);end=at+size;entries=0
  while at<end:
   count=struct.unpack_from('<I',raw,at)[0];at+=4+count*28;voice=struct.unpack_from('<I',raw,at)[0];at+=4+voice;entries+=1
  self.assertEqual(at,end);self.assertEqual(entries,1,'HistoryRecord re-recorded already consumed glyph records')
if __name__=='__main__':
 parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--exe',type=Path,required=True);args,remaining=parser.parse_known_args();EXE=args.exe.resolve();unittest.main(argv=[__file__,*remaining])
