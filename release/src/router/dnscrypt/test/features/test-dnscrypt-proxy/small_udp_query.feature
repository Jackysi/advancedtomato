Feature: Small UDP query

  A query that fits in a small UDP packet.
  
  Scenario: query an existing name.
  
    Given a working server proxy on 212.47.228.136
    And a running dnscrypt proxy with options "--edns-payload-size=0 -R dnscrypt.org-fr"
    When a client asks dnscrypt-proxy for "test-ff.dnscrypt.org"
    Then dnscrypt-proxy returns "255.255.255.255"

  Scenario: query a nonexistent name.
  
    Given a working server proxy on 212.47.228.136
    And a running dnscrypt proxy with options "--edns-payload-size=0 -R dnscrypt.org-fr"
    When a client asks dnscrypt-proxy for "test-nonexistent.dnscrypt.org"
    Then dnscrypt-proxy returns a NXDOMAIN answer
