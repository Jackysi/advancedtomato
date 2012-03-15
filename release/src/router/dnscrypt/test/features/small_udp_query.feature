Feature: Small UDP query

  A query that fits in a small UDP packet.
  
  Scenario: query an existing name.
  
    Given a working opendnscache on 208.67.220.220
    And a running dnscrypt proxy
    When a client asks opendnscache-proxy for "resolver1.opendns.com"
    Then opendnscache-proxy returns "208.67.222.222"

  Scenario: query a nonexistent name.
  
    Given a working opendnscache on 208.67.220.220
    And a running dnscrypt proxy
    When a client asks opendnscache-proxy for "nonexistent.opendns.com"
    Then opendnscache-proxy returns a NXDOMAIN answer
