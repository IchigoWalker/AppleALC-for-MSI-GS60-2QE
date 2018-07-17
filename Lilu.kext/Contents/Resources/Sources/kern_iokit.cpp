//
//  kern_iokit.cpp
//  Lilu
//
//  Copyright © 2016-2017 vit9696. All rights reserved.
//

#include <Headers/kern_config.hpp>
#include <Headers/kern_iokit.hpp>
#include <Headers/kern_util.hpp>
#include <Headers/kern_patcher.hpp>

#include "Library/LegacyIOService.h"
#include <libkern/c++/OSSerialize.h>
#include <IOKit/IORegistryEntry.h>
#include <IOKit/IODeviceTreeSupport.h>

namespace WIOKit {

	OSSerialize *getProperty(IORegistryEntry *entry, const char *property) {
		auto value = entry->getProperty(property);
		if (value) {
			auto s = OSSerialize::withCapacity(PAGE_SIZE);
			if (value->serialize(s)) {
				return s;
			} else {
				SYSLOG("iokit @ failed to serialise %s property", property);
				s->release();
			}
		} else {
			DBGLOG("iokit @ failed to get %s property", property);
		}
		return nullptr;
	}
	
	int getComputerModel() {
		auto entry = IORegistryEntry::fromPath("/", gIODTPlane);
		if (entry) {
			auto prop =  entry->getProperty("compatible");
			if (prop) {
				auto data = OSDynamicCast(OSData, prop);
				if (data) {
					//TODO: make this more reliable
					if (strstr(static_cast<const char *>(data->getBytesNoCopy()), "Book", strlen("Book"))) {
						return ComputerModel::ComputerLaptop;
					} else {
						return ComputerModel::ComputerDesktop;
					}
				} else {
					DBGLOG("iokit @ compatible property has invalid format");
				}
			} else {
				DBGLOG("iokit @ failed to get compatible property");
			}
		}
		DBGLOG("iokit @ failed to get DT entry");
		return ComputerModel::ComputerAny;
	}
	
	IORegistryEntry *findEntryByPrefix(const char *path, const char *prefix, const IORegistryPlane *plane, bool (*proc)(void *, IORegistryEntry *), bool brute, void *user) {
		auto entry = IORegistryEntry::fromPath(path, plane);
		if (entry) {
			auto res = findEntryByPrefix(entry, prefix, plane, proc, brute, user);
			entry->release();
			return res;
		}
		DBGLOG("iokit @ failed to get %s entry", path);
		return nullptr;
	}
	

	IORegistryEntry *findEntryByPrefix(IORegistryEntry *entry, const char *prefix, const IORegistryPlane *plane, bool (*proc)(void *, IORegistryEntry *), bool brute, void *user) {
		bool found {false};
		IORegistryEntry *res {nullptr};
		
		size_t bruteCount {0};
		
		do {
			bruteCount++;
			auto iterator = entry->getChildIterator(plane);
			
			if (iterator) {
				size_t len = strlen(prefix);
				while ((res = OSDynamicCast(IORegistryEntry, iterator->getNextObject())) != nullptr) {
					const char *resname = res->getName();
					
					//DBGLOG("iokit @ iterating over %s", resname);
					if (!strncmp(prefix, resname, len)) {
						found = proc ? proc(user, res) : true;
						if (found) {
							if (bruteCount > 1)
								DBGLOG("iokit @ bruted %s value in %zu attempts", prefix, bruteCount);
							if (!proc) {
								break;
							}
						}
					}
				}
				
				iterator->release();
			} else {
				SYSLOG("iokit @ failed to iterate over entry");
				return nullptr;
			}
			
		} while (brute && bruteCount < bruteMax && !found);
		
		if (!found)
			DBGLOG("iokit @ failed to find %s", prefix);
		return proc ? nullptr : res;
	}
}
