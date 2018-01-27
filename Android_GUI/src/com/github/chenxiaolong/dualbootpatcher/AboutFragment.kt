/*
 * Copyright (C) 2014-2015  Andrew Gunnerson <andrewgunnerson@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

package com.github.chenxiaolong.dualbootpatcher

import android.os.Bundle
import android.support.v4.app.Fragment
import android.text.Html
import android.text.method.LinkMovementMethod
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.TextView

class AboutFragment : Fragment() {
    override fun onCreateView(inflater: LayoutInflater, container: ViewGroup?,
                              savedInstanceState: Bundle?): View? {
        return inflater.inflate(R.layout.fragment_about, container, false)
    }

    override fun onActivityCreated(savedInstanceState: Bundle?) {
        super.onActivityCreated(savedInstanceState)

        val name: TextView = activity!!.findViewById(R.id.about_name)
        name.setText(BuildConfig.APP_NAME_RESOURCE)

        val version: TextView = activity!!.findViewById(R.id.about_version)
        version.text = String.format(getString(R.string.version), BuildConfig.VERSION_NAME)

        val credits: TextView = activity!!.findViewById(R.id.about_credits)

        val linkable = "<a href=\"%s\">%s</a>\n"
        val newline = "<br />"
        val separator = " | "

        val creditsText = getString(R.string.credits)
        val sourceCode = String.format(linkable,
                getString(R.string.url_source_code),
                getString(R.string.link_source_code))
        val xdaThread = String.format(linkable,
                getString(R.string.url_xda_thread),
                getString(R.string.link_xda_thread))
        val licenses = String.format(linkable,
                getString(R.string.url_licenses),
                getString(R.string.link_licenses))

        credits.text = Html.fromHtml(creditsText + newline + newline
                + sourceCode + separator + xdaThread + separator + licenses)
        credits.movementMethod = LinkMovementMethod.getInstance()
    }

    companion object {
        val FRAGMENT_TAG: String = AboutFragment::class.java.canonicalName

        fun newInstance(): AboutFragment {
            return AboutFragment()
        }
    }
}