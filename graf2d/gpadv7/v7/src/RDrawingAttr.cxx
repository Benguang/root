/// \file RDrawingAttrBase.cxx
/// \ingroup Gpad ROOT7
/// \author Axel Naumann <axel@cern.ch>
/// \date 2017-09-26
/// \warning This is part of the ROOT 7 prototype! It will change without notice. It might trigger earthquakes. Feedback
/// is welcome!

/*************************************************************************
 * Copyright (C) 1995-2018, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#include "ROOT/RDrawingAttr.hxx"

#include "ROOT/RDrawingOptsBase.hxx"
#include "ROOT/TLogger.hxx"

#include <algorithm>
#include <iterator>

ROOT::Experimental::RDrawingAttrBase::RDrawingAttrBase(const Name &name, const RDrawingAttrBase &parent) :
   fPath(parent.GetPath() + name), fHolder(parent.GetHolderPtr())
{
}

ROOT::Experimental::RDrawingAttrBase::RDrawingAttrBase(FromOption_t, const Name &name, RDrawingOptsBase &opts) :
   fPath{name.fStr}, fHolder(opts.GetHolder())
{
}

ROOT::Experimental::RDrawingAttrBase &ROOT::Experimental::RDrawingAttrBase::operator=(const RDrawingAttrBase& rhs)
{
   auto otherHolder = rhs.fHolder.lock();
   if (!otherHolder)
      return *this;

   auto thisHolder = fHolder.lock();
   if (!thisHolder)
      return *this;

   // First, remove all attributes in fPath; we will replace them with what's in rhs (if any).
   thisHolder->EraseAttributesInPath(fPath);
   thisHolder->CopyAttributesInPath(fPath, *otherHolder, rhs.fPath);
   return *this;
}

void ROOT::Experimental::RDrawingAttrBase::SetValueString(const Name &name, const std::string &strVal)
{
   if (auto holder = GetHolderPtr().lock())
      holder->At(GetPath() + name) = strVal;
}

std::string ROOT::Experimental::RDrawingAttrBase::GetValueString(const Path &path) const
{
   auto holder = GetHolderPtr().lock();
   if (!holder)
      return "";

   if (const std::string *pStr = holder->AtIf(path))
      return *pStr;
   return holder->GetAttrFromStyle(path);
}

bool ROOT::Experimental::RDrawingAttrBase::IsFromStyle(const Path& path) const
{
   auto holder = GetHolderPtr().lock();
   if (!holder)
      return "";

   return !holder->AtIf(path);
}

bool ROOT::Experimental::RDrawingAttrBase::IsFromStyle(const Name& name) const
{
   return IsFromStyle(GetPath() + name);
}

bool ROOT::Experimental::RDrawingAttrBase::operator==(const RDrawingAttrBase &other) const
{
   auto thisHolder = GetHolderPtr().lock();
   auto otherHolder = other.GetHolderPtr().lock();
   if (!thisHolder && !otherHolder)
      return true;
   if (!thisHolder != !otherHolder)
      return false;

   // We have valid holders for both.
   return thisHolder->Equal(*otherHolder.get(), GetPath(), other.GetPath());
}

float ROOT::Experimental::FromAttributeString(const std::string &val, const std::string &/*name*/, float *)
{
   return std::stof(val);
}

double ROOT::Experimental::FromAttributeString(const std::string &val, const std::string &/*name*/, double *)
{
   return std::stod(val);
}

int ROOT::Experimental::FromAttributeString(const std::string &val, const std::string &/*name*/, int*)
{
   return std::stoi(val);
}

std::string ROOT::Experimental::ToAttributeString(float val)
{
   return std::to_string(val);
}

std::string ROOT::Experimental::ToAttributeString(double val)
{
   return std::to_string(val);
}

std::string ROOT::Experimental::ToAttributeString(int val)
{
   return std::to_string(val);
}

const std::string *ROOT::Experimental::RDrawingAttrHolder::AtIf(const Path_t &path) const
{
   auto it = fAttrNameVals.find(path.fStr);
   if (it != fAttrNameVals.end())
      return &it->second;
   return nullptr;
}

std::string ROOT::Experimental::RDrawingAttrHolder::GetAttrFromStyle(const Path_t &path)
{
   R__WARNING_HERE("Graf2d") << "Failed to get attribute for "
      << path.fStr << ": not yet implemented!";
   return "";
}

bool ROOT::Experimental::RDrawingAttrHolder::Equal(const RDrawingAttrHolder &other, const Path_t &thisPath, const Path_t &otherPath)
{
   std::vector<Map_t::const_iterator> thisIters = GetAttributesInPath(thisPath);
   std::vector<Map_t::const_iterator> otherIters = other.GetAttributesInPath(otherPath);

   if (thisIters.size() != otherIters.size())
      return false;

   for (auto thisIter: thisIters) {
      // thisIters and otherIters have equal size. If any element in thisIters does not exist
      // in other.fAttrNameVals then they are not equal (if other.fAttrNameVals has an entry that
      // does not exist in this->fAttrNameVals, there must also be a key in this->fAttrNameVals
      // that does not exist in other.fAttrNameVals or the counts of thisIters and otherIters
      // would differ).
      // If all keys' values are equal, thisIters and otherIters are equal.
      auto otherIter = other.fAttrNameVals.find(thisIter->first);
      if (otherIter == other.fAttrNameVals.end())
         return false;
      if (thisIter->second != otherIter->second)
         return false;
   }
   return true;
}

std::vector<ROOT::Experimental::RDrawingAttrHolder::Map_t::const_iterator>
ROOT::Experimental::RDrawingAttrHolder::GetAttributesInPath(const Path_t &path) const
{
   std::vector<Map_t::const_iterator> ret;
   const std::string &stem = path.fStr;
   for (auto i = fAttrNameVals.begin(), e = fAttrNameVals.end(); i !=e; ++i)
      if (i->first.compare(0, stem.length(), stem) == 0) {
         // Require i->first to be complete stem, or more but then stem followed by ".":
         // stem "a.b", i->first can be "a.b" or "a.b.c.d"
         if (stem.length() == i->first.length()
             || i->first[stem.length()] == '.')
         ret.emplace_back(i);
      }
   return ret;
}

void ROOT::Experimental::RDrawingAttrHolder::EraseAttributesInPath(const Path_t &path)
{
   // Iterators are stable under erase()ing!
   auto iters = GetAttributesInPath(path);
   for (auto iter: iters)
      fAttrNameVals.erase(iter);
}


void ROOT::Experimental::RDrawingAttrHolder::CopyAttributesInPath(const Path_t &targetPath, const RDrawingAttrHolder &source, const Path_t &sourcePath)
{
   auto sourceIters = source.GetAttributesInPath(sourcePath);
   if (targetPath != sourcePath) {
      for (auto sourceIter: sourceIters)
         fAttrNameVals.emplace(sourceIter->first, sourceIter->second);
   } else {
      for (auto sourceIter: sourceIters) {
         std::string newPath = targetPath.fStr + sourceIter->first.substr(sourcePath.fStr.length());
         fAttrNameVals.emplace(newPath, sourceIter->second);
      }
   }
}