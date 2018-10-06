#pragma once

#include <cstdarg>
#include <cmath>
#include <iostream>
#include <vector>
#include <stdexcept>

#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>



namespace XScreenSize {
	class XScreen {
		public:
			int scr, width, height, mmWidth, mmHeight;

			XScreen() : scr(-1), width(-1), height(-1), mmWidth(-1), mmHeight(-1) {}
			XScreen(Display* dpy, int newScr) {
				scr = newScr;
				width = DisplayWidth(dpy, scr);
				height = DisplayHeight(dpy, scr);
				mmWidth = DisplayWidthMM(dpy, scr);
				mmHeight = DisplayHeightMM(dpy, scr);
			}

			friend std::ostream& operator<<(std::ostream& os, const XScreen& m) {
				os << "screen " << m.scr << ": " << m.width << "px x " << m.height << "px";
				return os;
			}
	};



	enum class XrandrNameKind : int {
		name_none = 0,
		name_string = (1 << 0),
		name_xid = (1 << 1),
		name_index = (1 << 2),
		name_preferred = (1 << 3)
	};

	inline constexpr XrandrNameKind operator&(XrandrNameKind x, XrandrNameKind y) {
		return static_cast<XrandrNameKind>(static_cast<int>(x) & static_cast<int>(y));
	}

	inline constexpr XrandrNameKind operator|(XrandrNameKind x, XrandrNameKind y) {
		return static_cast<XrandrNameKind>(static_cast<int>(x) | static_cast<int>(y));
	}

	inline XrandrNameKind& operator&=(XrandrNameKind& x, XrandrNameKind y){
		x = x & y;
		return x;
	}

	inline XrandrNameKind& operator|=(XrandrNameKind& x, XrandrNameKind y) {
		x = x | y;
		return x;
	}



	class XrandrName {
		public:
			XrandrNameKind kind;
			std::string str;
			XID xid;
			int index;

			XrandrName() : kind(XrandrNameKind::name_none) {}
			XrandrName(XID newXID) { setXID(newXID); }

			void setString(const std::string& newStr) {
				kind |= XrandrNameKind::name_string;
				str = newStr;
			}

			void setXID(XID newXid) {
				kind |= XrandrNameKind::name_xid;
				xid = newXid;
			}

			void setIndex(int newIndex) {
				kind |= XrandrNameKind::name_index;
				index = newIndex;
			}

			bool matches(const XrandrName& other) {
				XrandrNameKind common = other.kind & kind;
				if(static_cast<int>(common & XrandrNameKind::name_xid) && other.xid == xid) return true;
				if(static_cast<int>(common & XrandrNameKind::name_string) && other.str == str) return true;
				if(static_cast<int>(common & XrandrNameKind::name_index) && other.index == index) return true;
				return false;
			}
	};



	class XrandrCRTController {
		public:
			XrandrName crtc;
			XRRCrtcInfo* crtc_info;
			XRRModeInfo* mode_info;
	};



	class XrandrOutput {
		public:
			XrandrName output;
			XRROutputInfo* output_info;
			
			XrandrName crtc;
			XrandrCRTController* crtc_info;
			
			XrandrName mode;
			XRRModeInfo *mode_info;

			std::string name, connection;
      bool has_details;
			double refresh;
			int x, y, width, height, mmWidth, mmHeight;

			XrandrOutput() : output_info(nullptr), crtc_info(nullptr), mode_info(nullptr), has_details(false), refresh(0.0), x(-1), y(-1), width(-1), height(-1), mmWidth(-1), mmHeight(-1) {}

			bool can_use_crtc(const XrandrCRTController& crtc) {
				for(int c = 0; c < output_info->ncrtc; c++)
					if(output_info->crtcs[c] == crtc.crtc.xid)
						return true;
				return false;
			}

			friend std::ostream& operator<<(std::ostream& os, const XrandrOutput& m) {
				os << m.name << " [" << m.connection << "]";
				if(m.has_details)
					os << " at " << m.refresh << "Hz: " << m.width << "x" << m.height << "+" << m.x << "+" << m.y << ", " << m.mmWidth << "mm x " << m.mmHeight << "mm";
				return os;
			}
	};



	class Getter {
		private:
			Display* dpy;
			Window	root;
			XRRScreenResources* res;
			bool current = false;

			XScreen screen;
			std::vector<XrandrCRTController> crtcs;
			std::vector<XrandrOutput> outputs;

			std::runtime_error fatal(const char *format, ...) {
				char buf[1024];
				va_list args;
				va_start(args, format);
				vsnprintf(buf, sizeof(buf), format, args);
				va_end(args);
				return std::runtime_error(buf);
			}
			
			// vertical refresh frequency in Hz
			double mode_refresh(XRRModeInfo *mode_info) {
				double rate;

				if (mode_info->hTotal && mode_info->vTotal)
					rate = ((double)mode_info->dotClock / ((double)mode_info->hTotal * (double)mode_info->vTotal));
				else
					rate = 0;
				return rate;
			}

			XrandrCRTController* find_crtc(const XrandrName& name) {
				for(size_t c = 0; c < crtcs.size(); c++)
					if(crtcs[c].crtc.matches(name))
						return &crtcs[c];
				return nullptr;
			}

			XrandrCRTController* find_crtc_by_xid(RRCrtc crtc) {
				return find_crtc(XrandrName(crtc));
			}

			XRRModeInfo* find_mode(const XrandrName& name, double refresh) {
				XRRModeInfo	*best = nullptr;
				double bestDist = 0;

				for (int m = 0; m < res->nmode; m++) {
					XRRModeInfo *mode = &res->modes[m];
					if(static_cast<int>(name.kind & XrandrNameKind::name_xid) && name.xid == mode->id) {
						best = mode;
						break;
					}
					if(static_cast<int>(name.kind & XrandrNameKind::name_string) && name.str == mode->name) {
						double dist = refresh ? fabs(mode_refresh(mode) - refresh) : 0;
						if(!best || dist < bestDist) {
							bestDist = dist;
							best = mode;
						}
					}
				}
				return best;
			}

			XRRModeInfo* find_mode_by_xid(RRMode mode) {
				XrandrName mode_name{};
				mode_name.setXID(mode);
				return find_mode(mode_name, 0);
			}

			XRRModeInfo* find_mode_for_output(const XrandrOutput& output, const XrandrName& name) {
				XRROutputInfo *output_info = output.output_info;
				XRRModeInfo *best = nullptr;
				double bestDist = 0;

				for(int m = 0; m < output_info->nmode; m++) {
					XRRModeInfo *mode;
					mode = find_mode_by_xid (output_info->modes[m]);
					if(!mode) continue;
					if(static_cast<int>(name.kind & XrandrNameKind::name_xid) && name.xid == mode->id) {
						best = mode;
						break;
					}
					if(static_cast<int>(name.kind & XrandrNameKind::name_string) && name.str == mode->name) {
						// stay away from doublescan modes unless refresh rate is specified
						if(!output.refresh && (mode->modeFlags & RR_DoubleScan))
							continue;

						double dist = output.refresh ? fabs(mode_refresh(mode) - output.refresh) : 0;
						if(!best || dist < bestDist) {
							bestDist = dist;
							best = mode;
						}
					}
				}
				return best;
			}

			XrandrOutput create_output_info(RROutput xid, XRROutputInfo* output_info) {
				XrandrOutput output;
				std::vector<std::string> connection = {
					"connected",
					"disconnected",
					"unknown connection"
				};
				
				// set output name and info
				if (!static_cast<int>(output.output.kind & XrandrNameKind::name_xid))
					output.output.setXID(xid);
				if (!static_cast<int>(output.output.kind & XrandrNameKind::name_string))
					output.output.setString(output_info->name);
				output.output_info = output_info;

				// set crtc name and info
				output.crtc.setXID(output_info->crtc);

				if(output.crtc.kind == XrandrNameKind::name_xid && output.crtc.xid == None)
					output.crtc_info = nullptr;
				else {
					output.crtc_info = find_crtc(output.crtc);
					if(!output.crtc_info) {
						if(static_cast<int>(output.crtc.kind & XrandrNameKind::name_xid))
							throw fatal("cannot find crtc 0x%x", output.crtc.xid);
						if(static_cast<int>(output.crtc.kind & XrandrNameKind::name_index))
							throw fatal("cannot find crtc %d", output.crtc.index);
					}
					if(!output.can_use_crtc(*(output.crtc_info)))
						throw fatal("output %s cannot use crtc 0x%x", output.output.str.c_str(), output.crtc_info->crtc.xid);
				}

				// set mode name and info
				XrandrCRTController* crtc = find_crtc_by_xid(output_info->crtc);
				if(crtc && crtc->crtc_info)
					output.mode.setXID(crtc->crtc_info->mode);
				else if(output.crtc_info)
					output.mode.setXID(output.crtc_info->crtc_info->mode);
				else
					output.mode.setXID(None);
				if(output.mode.xid) {
					output.mode_info = find_mode_by_xid(output.mode.xid);
					if (!output.mode_info)
						throw fatal("server did not report mode 0x%x for output %s", output.mode.xid, output.output.str.c_str());
				}
				else
					output.mode_info = nullptr;

				// extract information
				output.name = output_info->name;
				output.connection = connection[output_info->connection];
				output.x = output.crtc_info ? output.crtc_info->crtc_info->x : 0;
				output.y = output.crtc_info ? output.crtc_info->crtc_info->y : 0;
				if(output.mode_info != nullptr) {
					output.has_details = true;
					output.refresh = mode_refresh(output.mode_info);
					output.width = output.mode_info->width;
					output.height = output.mode_info->height;
					output.mmWidth = (int)output_info->mm_width;
					output.mmHeight = (int)output_info->mm_height;
				}

				return output;
			}

			void get_screen(bool current) {
				if(current)
					res = XRRGetScreenResourcesCurrent(dpy, root);
				else
					res = XRRGetScreenResources(dpy, root);
				if(!res) throw fatal("could not get screen resources");
			}

			void get_crtcs() {
				for(int c = 0; c < res->ncrtc; c++) {
					XrandrCRTController crtc{};
					crtc.crtc.setXID(res->crtcs[c]);
					crtc.crtc.setIndex(c);

					XRRCrtcInfo *crtc_info = XRRGetCrtcInfo(dpy, res, res->crtcs[c]);
					if(!crtc_info) throw fatal("could not get crtc 0x%x information", res->crtcs[c]);
					
					crtc.crtc_info = crtc_info;
					if(crtc_info->mode == None)
						crtc.mode_info = nullptr;
					crtcs.push_back(crtc);
				}
			}


			// use current output state to complete the output list
			void get_outputs() {
				for (int o = 0; o < res->noutput; o++) {
					XRROutputInfo *output_info = XRRGetOutputInfo(dpy, res, res->outputs[o]);
					if (!output_info) throw fatal("could not get output 0x%x information", res->outputs[o]);
					outputs.push_back(create_output_info(res->outputs[o], output_info));
				}
			}
		
		public:
			Getter(char* display_name = nullptr, int screenNum = -1) {
				int event_base, error_base;
				int major, minor;

				dpy = XOpenDisplay(display_name);
				if(dpy == nullptr)
					throw fatal("can't open display %s", XDisplayName(display_name));
				
				if(screenNum < 0)
					screenNum = DefaultScreen(dpy);
				if(screenNum >= ScreenCount(dpy))
					throw fatal("invalid screen number %d, display has %d", screenNum, ScreenCount(dpy));

				root = RootWindow(dpy, screenNum);
				if(!XRRQueryExtension(dpy, &event_base, &error_base) || !XRRQueryVersion(dpy, &major, &minor))
					throw fatal("RandR extension missing");
				if(!(major > 1 || (major == 1 && minor >= 2)))
					throw fatal("RandR version >1.2 is required, having %d.%d", major, minor);

				screen = XScreen(dpy, screenNum);
				get_screen(current);
				get_crtcs();
				get_outputs();	
			}

			const XScreen& getScreen() {
				return screen;
			}

			const std::vector<XrandrOutput>& getOutputs() {
				return outputs;
			}
	};
}
