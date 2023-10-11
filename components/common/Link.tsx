import * as React from "react";

export const defaultAllowedSchemes = ['http:', 'https:']

type Props = React.DetailedHTMLProps<React.AnchorHTMLAttributes<HTMLAnchorElement>, HTMLAnchorElement> & {
  allowedSchemes?: string[]
}

export const validateScheme = (href: string | undefined, allowedSchemes: string[] = defaultAllowedSchemes) => {
  // Special case the empty url - it shouldn't crash anything
  if (!href) return

  const url = new URL(href)
  if (!allowedSchemes.includes(url.protocol)) {
    throw new Error(`Unsupported scheme ${url.protocol} on url ${href}`)
  }
}

/**
 * Identical to the `a` element, except that it validates the HREF points
 * to an allowed scheme. By default, allowed schemes are http: or https:,
 * but this can be overridden.
 */
export default function Link({ href, allowedSchemes, ...rest }: Props) {
  validateScheme(href, allowedSchemes ?? defaultAllowedSchemes)
  return <a {...rest} />
}
